/* Copyright 2007 David Ungar, 2024-2026 Russell Allen
   See the LICENSE file for license information. */

// ARM64 macOS: Cocoa implementation of quartzWindow
// Replaces Carbon APIs with NSWindow/NSEvent/CoreText

# pragma implementation

# if defined(QUARTZ_LIB) && defined(__aarch64__)

// The .mm file does not get the precompiled header (it's C++ only, not ObjC++).
// Include AppKit FIRST so system types are defined, then include Self headers.
// quartzWindow.hh will detect already-defined system types and skip its versions.
//
// Workaround: Xcode does inject the PCH for .mm files via -include, so util.hh's
// `const int32 K = 1024;` is already in scope. macOS SDK 26.5's NSDictionary.h
// declares a generic `<K, V>` whose unqualified `K` then becomes ambiguous with
// the global. Rename the SDK's `K` lexically via a macro just for these imports.
//
// This did not arise before the AVP port because the macOS build used
// quartzWindow.cpp (Carbon, pure C++); no translation unit pulled both util.hh
// and Foundation. The port introduced this .mm (Cocoa, ObjC++) for ARM64, and
// SDK 26.5 added the `<K, V>` generic that triggers the ambiguity.
#define K _Self_NSDict_K
#import <AppKit/AppKit.h>
#undef K
#import <CoreText/CoreText.h>
#import <QuartzCore/QuartzCore.h>
#import <IOSurface/IOSurface.h>

// Undef Cocoa/AppKit macros that conflict with Self
#undef assert
#undef check
#undef verify

// Now include Self precompiled header and specific includes.
// Guard against double-inclusion: Xcode's PCH mechanism auto-includes the
// prefix header for ALL sources (including .mm), whereas Makefile builds
// only inject it for .cpp/.c/.hh files.  The headers lack include guards.
// Test for a macro defined by config.hh (the first file in the PCH).
// -- dmu & claude, 5/26
# ifndef SPARC_ARCH
#  include "_precompiled.hh"
# endif
# include "_quartzWindow.cpp.incl"


// ======================================================================
// WindowSet implementation
// ======================================================================

int       WindowSet::_num_windows = 0;
WindowSet_WindowPtr WindowSet::_my_windows[WindowSet::_max_windows];

void WindowSet::add_window(WindowSet_WindowPtr w) {
  if (_num_windows >= _max_windows)  fatal("too many");
  _my_windows[_num_windows++] = w;
}

void WindowSet::rm_window(WindowSet_WindowPtr w) {
  int i;
  for (i = 0;  _my_windows[i] != w;  ++i)
    if (i >= _num_windows)
      fatal("did not find _window");
  if (i == _num_windows - 1)   --_num_windows;
  else                         _my_windows[i] = _my_windows[--_num_windows];
}

bool WindowSet::includes_window(WindowSet_WindowPtr w) {
  for (int i = 0;  i < _num_windows;  ++i)
    if (_my_windows[i] == w) {
      if (i != 0) {
        WindowSet_WindowPtr w0 = _my_windows[0];
        _my_windows[0] = w;
        _my_windows[i] = w0;
      }
      return true;
    }
  return false;
}


// Sleep/wake guard — declared early so all ObjC classes can see it.
static _Atomic(bool) system_is_sleeping = false;
static _Atomic(uint32_t) sleep_generation = 0;


// ======================================================================
// SelfContentView - NSView subclass that provides CGContext
// ======================================================================

@interface SelfContentView : NSView
@property (nonatomic, assign) QuartzWindow* quartzWindow;
@end

// ======================================================================
// Cross-window drag support
// ----------------------------------------------------------------------
// Cocoa delivers mouseDragged:/mouseUp: only to the window where the drag
// began, but the Self ui2 world was written for X11, which routes pointer
// motion to the window UNDER the cursor and emits Enter/LeaveNotify when the
// pointer crosses a window boundary.  To make a morph drag hand off between
// windows the way it does on X11, we (a) route a held-button drag to the Self
// window under the cursor and (b) synthesize windowLeave/windowEnter on each
// transition.  windowLeave is a mouse event whose part code is inDesk (the
// Self quartz event mapper turns that into 'windowLeave'); windowEnter reuses
// the window handleActivate kind (which the mapper turns into 'windowEnter').
//
// -- claude & dmu, 5/26
// ======================================================================

// Map a Cocoa window number to the QuartzWindow that owns it, or NULL if it is
// not one of our Self content windows (e.g. another app, or the desktop).
static QuartzWindow* selfWindowForNumber(NSInteger n) {
    if (n <= 0) return NULL;
    NSWindow* w = [NSApp windowWithWindowNumber:n];
    if (!w) return NULL;
    NSView* v = [w contentView];
    if (![v isKindOfClass:[SelfContentView class]]) return NULL;
    return [(SelfContentView*)v quartzWindow];
}

// Fill in the location/window params shared by both synthetic crossing events.
static void setCrossingLocation(OpaqueEventRef* e, NSWindow* nsWin, QuartzWindow* qw,
                                NSPoint screenPt, double globalX, double globalY) {
    e->setParam_point(kEventParamMouseLocation, globalX, globalY);
    NSPoint loc = [nsWin convertPointFromScreen:screenPt];
    NSRect  b   = [[nsWin contentView] bounds];
    e->setParam_point(kEventParamWindowMouseLocation,
                      qw->inset_left() + loc.x,
                      qw->inset_top()  + (b.size.height - loc.y));
    e->setParam_ptr(kEventParamWindowRef, typeWindowRef, qw->my_window());
}

// windowLeave: a mouse-moved event tagged with part code inDesk.
static void postWindowLeave(NSInteger num, NSPoint screenPt, double gX, double gY, double t) {
    NSWindow*     nsWin = [NSApp windowWithWindowNumber:num];
    QuartzWindow* qw    = selfWindowForNumber(num);
    if (!nsWin || !qw) return;
    OpaqueEventRef* e = new OpaqueEventRef();
    e->eventClass = kEventClassMouse;
    e->eventKind  = kEventMouseMoved;
    e->eventTime  = t;
    setCrossingLocation(e, nsWin, qw, screenPt, gX, gY);
    e->setParam_uint32(kEventParamMouseChord,    typeUInt32, 0);
    e->setParam_uint32(kEventParamWindowDefPart, typeUInt32, kPartInDesk);
    qw->put_event(e);
    e->release();
}

// windowEnter: a window handleActivate event.
static void postWindowEnter(NSInteger num, NSPoint screenPt, double gX, double gY, double t) {
    NSWindow*     nsWin = [NSApp windowWithWindowNumber:num];
    QuartzWindow* qw    = selfWindowForNumber(num);
    if (!nsWin || !qw) return;
    OpaqueEventRef* e = new OpaqueEventRef();
    e->eventClass = kEventClassWindow;
    e->eventKind  = kEventWindowHandleActivate;
    e->eventTime  = t;
    setCrossingLocation(e, nsWin, qw, screenPt, gX, gY);
    qw->put_event(e);
    e->release();
}

// Route a held-button drag to the Self window currently under the cursor (X11
// delivers motion to the window under the pointer; Cocoa keeps it on the
// mouse-down window).  Emits windowLeave/windowEnter as the cursor crosses
// between Self windows, updates currentWindowNumberForDrag (the window the drag is routed to now),
// and leaves destNS/destQW unchanged unless the cursor is over a Self window.
// -- claude & dmu 5/26
static NSInteger destinationSelfWindow(
        NSInteger originWindowNumber, NSPoint screenPt, double globalX, double globalY, double t,
        NSInteger currentWindowNumberForDrag) {
    NSInteger     underNum = [NSWindow windowNumberAtPoint:screenPt belowWindowWithWindowNumber:0];
    QuartzWindow* overQW   = selfWindowForNumber(underNum);
    // Over a non-Self window (other app / desktop) → stay with the origin.
    NSInteger     destSelfWindow   = (overQW && underNum != originWindowNumber) ? underNum : originWindowNumber;
    if (destSelfWindow == currentWindowNumberForDrag)
      return currentWindowNumberForDrag;
  postWindowLeave(currentWindowNumberForDrag, screenPt, globalX, globalY, t);
  postWindowEnter(destSelfWindow, screenPt, globalX, globalY, t);
  return destSelfWindow;
}

@implementation SelfContentView

- (BOOL)isFlipped {
    return NO; // Keep CoreGraphics coordinate system (origin at bottom-left)
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}

- (void)mouseDown:(NSEvent *)event       { [self handleMouseEvent:event kind:kEventMouseDown]; }
- (void)mouseUp:(NSEvent *)event         { [self handleMouseEvent:event kind:kEventMouseUp]; }
- (void)mouseMoved:(NSEvent *)event      { [self handleMouseEvent:event kind:kEventMouseMoved]; }
- (void)mouseDragged:(NSEvent *)event    { [self handleMouseEvent:event kind:kEventMouseDragged]; }
- (void)rightMouseDown:(NSEvent *)event  { [self handleMouseEvent:event kind:kEventMouseDown]; }
- (void)rightMouseUp:(NSEvent *)event    { [self handleMouseEvent:event kind:kEventMouseUp]; }
- (void)rightMouseDragged:(NSEvent *)event { [self handleMouseEvent:event kind:kEventMouseDragged]; }
- (void)otherMouseDown:(NSEvent *)event  { [self handleMouseEvent:event kind:kEventMouseDown]; }
- (void)otherMouseUp:(NSEvent *)event    { [self handleMouseEvent:event kind:kEventMouseUp]; }
- (void)otherMouseDragged:(NSEvent *)event { [self handleMouseEvent:event kind:kEventMouseDragged]; }

- (void)scrollWheel:(NSEvent *)event {
    if (!_quartzWindow) return;

    // Discard scroll events with no actual delta — these are gesture-phase
    // bookkeeping events (began/ended/cancelled) that macOS sends during
    // two-finger trackpad clicks, causing spurious redraws.
    if ([event deltaX] == 0 && [event deltaY] == 0) return;

    OpaqueEventRef* evt = new OpaqueEventRef();
    evt->eventClass = kEventClassMouse;
    evt->eventKind = kEventMouseWheelMoved;
    evt->eventTime = [event timestamp];

    // Global screen coordinates (top-left origin)
    NSPoint locInWindow = [event locationInWindow];
    NSPoint locOnScreen = [[self window] convertPointToScreen:locInWindow];
    CGFloat screenH = [[NSScreen mainScreen] frame].size.height;
    evt->setParam_point(kEventParamMouseLocation, locOnScreen.x, screenH - locOnScreen.y);

    // Window-local coordinates (structure-relative, top-left origin)
    NSRect contentBounds = [self bounds];
    int insetLeft = _quartzWindow->inset_left();
    int insetTop  = _quartzWindow->inset_top();
    evt->setParam_point(kEventParamWindowMouseLocation,
                        insetLeft + locInWindow.x,
                        insetTop + (contentBounds.size.height - locInWindow.y));

    // Wheel axis and delta
    // Carbon convention: kEventMouseWheelAxisX = 0, kEventMouseWheelAxisY = 1
    if ([event deltaY] != 0) {
        evt->setParam_uint32(kEventParamMouseWheelAxis, typeMouseWheelAxis, 1); // kEventMouseWheelAxisY
        evt->setParam_int32(kEventParamMouseWheelDelta, typeSInt32, (int32)[event deltaY]);
    } else if ([event deltaX] != 0) {
        evt->setParam_uint32(kEventParamMouseWheelAxis, typeMouseWheelAxis, 0); // kEventMouseWheelAxisX
        evt->setParam_int32(kEventParamMouseWheelDelta, typeSInt32, (int32)[event deltaX]);
    }

    // Modifier keys
    NSUInteger mods = [event modifierFlags];
    uint32 carbonMods = 0;
    if (mods & NSEventModifierFlagShift)    carbonMods |= (1 << 9);
    if (mods & NSEventModifierFlagControl)  carbonMods |= (1 << 12);
    if (mods & NSEventModifierFlagOption)   carbonMods |= (1 << 11);
    if (mods & NSEventModifierFlagCommand)  carbonMods |= (1 << 8);
    if (mods & NSEventModifierFlagCapsLock) carbonMods |= (1 << 10); // capsLock
    evt->setParam_uint32(kEventParamKeyModifiers, typeUInt32, carbonMods);

    // Window reference and part code
    evt->setParam_ptr(kEventParamWindowRef, typeWindowRef, _quartzWindow->my_window());
    evt->setParam_uint32(kEventParamWindowDefPart, typeUInt32, kPartInContent);

    _quartzWindow->put_event(evt);
    evt->release(); // put_event retains
}

- (void)keyDown:(NSEvent *)event {
    [self handleKeyEvent:event kind:kEventRawKeyDown];
}

- (void)keyUp:(NSEvent *)event {
    [self handleKeyEvent:event kind:kEventRawKeyUp];
}

- (void)flagsChanged:(NSEvent *)event {
    [self handleKeyEvent:event kind:kEventRawKeyModifiersChanged];
}

- (void)handleMouseEvent:(NSEvent *)event kind:(uint32)kind {
    if (!_quartzWindow) return;

    OpaqueEventRef* evt = new OpaqueEventRef();
    evt->eventClass = kEventClassMouse;
    evt->eventKind = kind;
    evt->eventTime = [event timestamp];

    // Global screen coordinates (top-left origin) — kEventParamMouseLocation
    NSPoint locInWindow = [event locationInWindow];
    NSPoint locOnScreen = [[self window] convertPointToScreen:locInWindow];
    CGFloat screenH = [[NSScreen mainScreen] frame].size.height;
    double globalX = locOnScreen.x;
    double globalY = screenH - locOnScreen.y;
    evt->setParam_point(kEventParamMouseLocation, globalX, globalY);

    // Window-local coordinates (kEventParamWindowMouseLocation) and the window
    // reference are set below, after the destination window is determined: a
    // cross-window drag re-targets the event to the window under the cursor, so
    // those must be computed relative to THAT window.
    // claude & dmu, 5/26

    // Button number: Carbon uses 1=left, 2=right, 3=middle
    uint16 button = 1;
    switch ([event type]) {
        case NSEventTypeRightMouseDown:
        case NSEventTypeRightMouseUp:
        case NSEventTypeRightMouseDragged:
            button = 2;
            break;
        case NSEventTypeOtherMouseDown:
        case NSEventTypeOtherMouseUp:
        case NSEventTypeOtherMouseDragged:
            button = (uint16)[event buttonNumber] + 1;
            break;
        default:
            button = 1;
            break;
    }
    evt->setParam_uint32(kEventParamMouseButton, typeMouseButton, button);

    // Mouse chord: bitmask of buttons currently held
    // Carbon: bit 0 = primary, bit 1 = secondary, bit 2 = tertiary
    NSUInteger pressed = [NSEvent pressedMouseButtons];
    uint32 chord = 0;
    if (pressed & (1 << 0)) chord |= 1;  // left
    if (pressed & (1 << 1)) chord |= 2;  // right
    if (pressed & (1 << 2)) chord |= 4;  // middle
    evt->setParam_uint32(kEventParamMouseChord, typeUInt32, chord);

    // Click count
    NSInteger clickCount = 0;
    if (kind == kEventMouseDown || kind == kEventMouseUp) {
        clickCount = [event clickCount];
    }
    evt->setParam_uint32(kEventParamClickCount, typeUInt32, (uint32)clickCount);

    // Modifier keys
    NSUInteger mods = [event modifierFlags];
    uint32 carbonMods = 0;
    if (mods & NSEventModifierFlagShift)    carbonMods |= (1 << 9);  // shiftKey
    if (mods & NSEventModifierFlagControl)  carbonMods |= (1 << 12); // controlKey
    if (mods & NSEventModifierFlagOption)   carbonMods |= (1 << 11); // optionKey
    if (mods & NSEventModifierFlagCommand)  carbonMods |= (1 << 8);  // cmdKey
    if (mods & NSEventModifierFlagCapsLock) carbonMods |= (1 << 10); // capsLock
    evt->setParam_uint32(kEventParamKeyModifiers, typeUInt32, carbonMods);

    // ---- Cross-window drag routing (mirror X11) --------------------------
    // While a button is held (and on the terminating mouseUp), deliver the
    // event to the Self window under the cursor rather than the window that
    // began the drag, and emit windowLeave/windowEnter as the cursor crosses
    // between Self windows so the carried morph is handed off.
    // claude & dmu, 5/26
    NSInteger originWindowNumber = [[self window] windowNumber];
    BOOL      dragging  = (chord != 0) || (kind == kEventMouseUp);

    static NSInteger dragsDestinationSelfWindow = 0;   // Self window the drag is routed to now
    if (kind == kEventMouseDown) dragsDestinationSelfWindow = originWindowNumber;

    NSWindow*     destNS = [self window];
    QuartzWindow* destQW = _quartzWindow;

  if (dragging) {
    dragsDestinationSelfWindow = destinationSelfWindow(originWindowNumber, locOnScreen, globalX, globalY,
                                    [event timestamp], dragsDestinationSelfWindow);
    NSWindow*     rNS = [NSApp windowWithWindowNumber:dragsDestinationSelfWindow];
    QuartzWindow* rQW = selfWindowForNumber(dragsDestinationSelfWindow);
    if (rNS && rQW) { destNS = rNS; destQW = rQW; }
  }

    // Window-local coordinates and window reference, relative to the destination.
    // claude & dmu, 5/26
    NSPoint destLoc    = [destNS convertPointFromScreen:locOnScreen];
    NSRect  destBounds = [[destNS contentView] bounds];
    evt->setParam_point(kEventParamWindowMouseLocation,
                        destQW->inset_left() + destLoc.x,
                        destQW->inset_top()  + (destBounds.size.height - destLoc.y));
    evt->setParam_ptr(kEventParamWindowRef, typeWindowRef, destQW->my_window());
    evt->setParam_uint32(kEventParamWindowDefPart, typeUInt32, kPartInContent);

    destQW->put_event(evt);
    evt->release(); // put_event retains
}

// Map Cocoa Unicode character codes to classic Mac/Carbon character codes.
// The Self world expects Carbon-style kEventParamKeyMacCharCodes values,
// but [NSEvent characters] returns different Unicode values for special keys.
static uint32 cocoaCharToMacCharCode(unichar ch) {
    switch (ch) {
        case NSBackspaceCharacter:                   return 0x08;
        case NSDeleteCharacter:                      return 0x08;
        case NSDeleteFunctionKey:                    return 0x7F;
        case NSUpArrowFunctionKey:                   return 0x1E;
        case NSDownArrowFunctionKey:                 return 0x1F;
        case NSLeftArrowFunctionKey:                 return 0x1C;
        case NSRightArrowFunctionKey:                return 0x1D;
        case NSHomeFunctionKey:                      return 0x01;
        case NSEndFunctionKey:                       return 0x04;
        case NSPageUpFunctionKey:                    return 0x0B;
        case NSPageDownFunctionKey:                  return 0x0C;
        case NSClearLineFunctionKey:                 return 0x0C;
        case NSF1FunctionKey:  case NSF2FunctionKey:
        case NSF3FunctionKey:  case NSF4FunctionKey:
        case NSF5FunctionKey:  case NSF6FunctionKey:
        case NSF7FunctionKey:  case NSF8FunctionKey:
        case NSF9FunctionKey:  case NSF10FunctionKey:
        case NSF11FunctionKey: case NSF12FunctionKey:
            return 0x10;
        default:
            return (uint32)ch;
    }
}

- (void)handleKeyEvent:(NSEvent *)event kind:(uint32)kind {
    if (!_quartzWindow) return;

    OpaqueEventRef* evt = new OpaqueEventRef();
    evt->eventClass = kEventClassKeyboard;
    evt->eventKind = kind;
    evt->eventTime = [event timestamp];

    // Key code
    evt->setParam_uint32(kEventParamKeyCode, typeUInt32, (uint32)[event keyCode]);

    // Character code
    if (kind != kEventRawKeyModifiersChanged) {
        NSString* chars = [event characters];
        if ([chars length] > 0) {
            uint32 charCode = cocoaCharToMacCharCode([chars characterAtIndex:0]);
            evt->setParam_uint32(kEventParamKeyMacCharCodes, typeUInt32, charCode);
        }
    }

    // Modifier keys
    NSUInteger mods = [event modifierFlags];
    uint32 carbonMods = 0;
    if (mods & NSEventModifierFlagShift)    carbonMods |= (1 << 9);
    if (mods & NSEventModifierFlagControl)  carbonMods |= (1 << 12);
    if (mods & NSEventModifierFlagOption)   carbonMods |= (1 << 11);
    if (mods & NSEventModifierFlagCommand)  carbonMods |= (1 << 8);
    if (mods & NSEventModifierFlagCapsLock) carbonMods |= (1 << 10); // capsLock
    evt->setParam_uint32(kEventParamKeyModifiers, typeUInt32, carbonMods);

    // Window reference
    evt->setParam_ptr(kEventParamWindowRef, typeWindowRef, _quartzWindow->my_window());

    _quartzWindow->put_event(evt);
    evt->release(); // put_event retains
}

- (BOOL)wantsUpdateLayer {
    return YES;
}

- (void)updateLayer {
    if (system_is_sleeping) return;
    if (!_quartzWindow) return;
    IOSurfaceRef surface = _quartzWindow->ioSurface();
    if (!surface) return;
    self.layer.contents = (__bridge id)surface;
    [self.layer performSelector:@selector(setContentsChanged)];
}

@end


// ======================================================================
// SelfSleepObserver - guard event loop during system sleep to avoid crashes
// ======================================================================

@interface SelfSleepObserver : NSObject
@end

@implementation SelfSleepObserver

- (void)systemWillSleep:(NSNotification *)notification {
    uint32_t gen = ++sleep_generation;
    system_is_sleeping = true;
    IntervalTimer::disable_all(true);
    // Mach time doesn't advance during sleep, so this fires ~2s after wake,
    // giving the display subsystem time to reinitialize.
    // Generation check ensures a newer sleep doesn't get cleared by a stale block.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)),
        dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
            if (sleep_generation == gen) {
                system_is_sleeping = false;
                IntervalTimer::enable_all();
            }
        });
}

@end


// ======================================================================
// SelfWindowDelegate - handles window lifecycle events
// ======================================================================

@interface SelfWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) QuartzWindow* quartzWindow;
@end

@implementation SelfWindowDelegate

- (void)windowDidResize:(NSNotification *)notification {
    if (_quartzWindow)
        _quartzWindow->queue_bounds_changed();
}

- (void)windowDidMove:(NSNotification *)notification {
    if (_quartzWindow) {
        OpaqueEventRef* evt = new OpaqueEventRef();
        evt->eventClass = kEventClassWindow;
        evt->eventKind = kEventWindowBoundsChanged;
        evt->eventTime = [[NSProcessInfo processInfo] systemUptime];
        evt->setParam_ptr(kEventParamWindowRef, typeWindowRef, _quartzWindow->my_window());
        _quartzWindow->put_event(evt);
        evt->release();
    }
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
    if (_quartzWindow) {
        _quartzWindow->set_was_closed();

        OpaqueEventRef* evt = new OpaqueEventRef();
        evt->eventClass = kEventClassWindow;
        evt->eventKind = kEventWindowClose;
        evt->eventTime = [[NSProcessInfo processInfo] systemUptime];
        evt->setParam_ptr(kEventParamWindowRef, typeWindowRef, _quartzWindow->my_window());
        _quartzWindow->put_event(evt);
        evt->release();
    }
    return NO; // Let Self handle the close
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    if (_quartzWindow) {
        OpaqueEventRef* evt = new OpaqueEventRef();
        evt->eventClass = kEventClassWindow;
        evt->eventKind = kEventWindowHandleActivate;
        evt->eventTime = [[NSProcessInfo processInfo] systemUptime];
        evt->setParam_ptr(kEventParamWindowRef, typeWindowRef, _quartzWindow->my_window());
        _quartzWindow->put_event(evt);
        evt->release();
    }
}

- (void)windowDidResignKey:(NSNotification *)notification {
    if (_quartzWindow) {
        OpaqueEventRef* evt = new OpaqueEventRef();
        evt->eventClass = kEventClassWindow;
        evt->eventKind = kEventWindowHandleDeactivate;
        evt->eventTime = [[NSProcessInfo processInfo] systemUptime];
        evt->setParam_ptr(kEventParamWindowRef, typeWindowRef, _quartzWindow->my_window());
        _quartzWindow->put_event(evt);
        evt->release();
    }
}

@end


// ======================================================================
// Bitmap blitting helper — bypasses drawRect for immediate display
// ======================================================================

static void blitIOSurfaceToView(SelfContentView* view, IOSurfaceRef surface) {
    if (system_is_sleeping) return;
    if (!view || !surface) return;
    CALayer* layer = [view layer];
    if (layer) {
        [CATransaction begin];
        [CATransaction setDisableActions:YES]; // no implicit animation
        layer.contents = (__bridge id)surface;
        [layer performSelector:@selector(setContentsChanged)];
        [CATransaction commit];
    }
}


// ======================================================================
// SelfAppDelegate - minimal NSApplicationDelegate
// Exists only to declare our state-restoration secure-coding policy, which
// silences AppKit's "implement applicationSupportsSecureRestorableState:"
// warning. We never restore window state, so YES is safe and asserts nothing
// we don't honor.
// -- claude & dmu 5/26
// ======================================================================

@interface SelfAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation SelfAppDelegate
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}
@end


// ======================================================================
// NSApplication initialization (must be called before any Cocoa use)
// ======================================================================

static bool cocoa_initialized = false;

// Diagnostic counter for the console test harness (SELF_TEST_DUMP_CONSOLE):
// how often the event queue has actually been pumped.
long g_quartz_pump_count = 0;

static void ensure_cocoa_initialized() {
  if (cocoa_initialized) return;
  cocoa_initialized = true;

# if COCOA_EXP
  return;  // NSApplicationMain has already initialized Cocoa
# endif

  @autoreleasepool {
    [NSApplication sharedApplication];
    // Install a minimal delegate before finishLaunching so AppKit can read our
    // secure-coding policy for state restoration. -- claude & dmu 5/26
    static SelfAppDelegate* appDelegate = [[SelfAppDelegate alloc] init];
    [NSApp setDelegate:appDelegate];

    // We don't restore window state; tell AppKit not to save/restore it so it
    // stops attempting restoration (the "restoreWindowWithIdentifier ...
    // className=(null)" log line). registerDefaults sets a volatile default,
    // so this never persists into the user's real preferences.
    // -- claude & dmu 5/26
    [[NSUserDefaults standardUserDefaults]
        registerDefaults:@{@"NSQuitAlwaysKeepsWindows": @NO}];

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    // Set an empty main menu before finishLaunching to prevent AppKit from
    // building the default menu, which triggers lazy loading of Writing Tools
    // and other frameworks — very slow under lldb due to dyld image notifications.
    // -- dmu & claude, 5/26
    [NSApp setMainMenu:[[NSMenu alloc] init]];
    [NSApp finishLaunching];

    // Register for sleep notification to guard the Cocoa event loop.
    // On sleep, we set a flag so check_carbon_events() skips pumping
    // the event loop (which would hit invalidated display state and crash).
    // The flag auto-clears ~2s after wake via dispatch_after.
    static SelfSleepObserver* sleepObserver = [[SelfSleepObserver alloc] init];
    [[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserver:sleepObserver
           selector:@selector(systemWillSleep:)
               name:NSWorkspaceWillSleepNotification
             object:nil];
  }
}


// ======================================================================
// QuartzWindow implementation
// ======================================================================

QuartzWindow::QuartzWindow() : AbstractPlatformWindow(), _evtQ() {
  _is_open = false;
  _ns_window = NULL;
  _ns_view = NULL;
  _bounds_changed = false;
  _was_closed = false;
  _quartz_win = NULL;
  myContext = NULL;
  _bitmapContext = NULL;
  _ioSurface = NULL;
  _bitmapWidth = 0;
  _bitmapHeight = 0;
  _default_ct_font = NULL;
  _color_space = NULL;
  memset(&_windowPtr, 0, sizeof(_windowPtr));
  memset(&_grafPtr, 0, sizeof(_grafPtr));
}


void QuartzWindow::destroyBitmapContext() {
  if (_bitmapContext) {
    CGContextRelease(_bitmapContext);
    _bitmapContext = NULL;
  }
  if (_ioSurface) {
    CFRelease(_ioSurface);
    _ioSurface = NULL;
  }
  _bitmapWidth = 0;
  _bitmapHeight = 0;
  myContext = NULL;
}

void QuartzWindow::ensureBitmapContext() {
  int w = width();
  int h = height();
  if (w <= 0) w = 1;
  if (h <= 0) h = 1;
  if (_bitmapContext && _bitmapWidth == w && _bitmapHeight == h)
    return;
  destroyBitmapContext();
  size_t bytesPerRow = ((size_t)w * 4 + 15) & ~(size_t)15;  // align to 16 bytes

  NSDictionary* props = @{
    (id)kIOSurfaceWidth:            @(w),
    (id)kIOSurfaceHeight:           @(h),
    (id)kIOSurfaceBytesPerElement:  @4,
    (id)kIOSurfaceBytesPerRow:      @(bytesPerRow),
    (id)kIOSurfacePixelFormat:      @((uint32_t)'BGRA'),
  };
  _ioSurface = IOSurfaceCreate((__bridge CFDictionaryRef)props);
  if (!_ioSurface) fatal("could not create IOSurface");

  IOSurfaceLock(_ioSurface, 0, NULL);
  void* baseAddr = IOSurfaceGetBaseAddress(_ioSurface);
  memset(baseAddr, 0, bytesPerRow * (size_t)h);
  _bitmapContext = CGBitmapContextCreate(
      baseAddr, w, h, 8, bytesPerRow,
      _color_space, kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
  if (!_bitmapContext) fatal("could not create bitmap context");
  IOSurfaceUnlock(_ioSurface, 0, NULL);

  _bitmapWidth = w;
  _bitmapHeight = h;
  // Clear to white so uncovered areas aren't black
  CGContextSetRGBFillColor(_bitmapContext, 1.0, 1.0, 1.0, 1.0);
  CGContextFillRect(_bitmapContext, CGRectMake(0, 0, w, h));
}


bool QuartzWindow::open( const char* /* display_name */,
                           int x, int y, int w, int h,
                           int min_w, int max_w, int min_h, int max_h,
                           const char* window_name,  const char* /*icon_name*/,
                           const char* font_name,    int   font_size ) {

  int options[8] = {
    kHIWindowBitCloseBox,
    kHIWindowBitZoomBox,
    kHIWindowBitCollapseBox,
    kHIWindowBitResizable,
    kHIWindowBitRoundBottomBarCorners,
    kHIWindowBitStandardHandler,
    0
  };

  if ( !open( kDocumentWindowClass, options,
             x, y, x + w, y + h, window_name, font_name, font_size))
    return false;
  if ( !change_size_hints(min_w, max_w, min_h, max_h)) { close();  return false; }
  init_font_info();
  activate();
  return true;
}


bool QuartzWindow::open(
                    uint32  wc,
                    int*    attrs,
                    int   left,
                    int   top,
                    int   right,
                    int   bottom,
                    const char* title,
                    const char* font_name,
                    int   font_size ) {

  ensure_cocoa_initialized();

  @autoreleasepool {
    // Create NSWindow
    // Note: Carbon used (left, top, right-left, bottom-top) but passed as (left, top, right, bottom)
    // The caller passes right=left+width, bottom=top+height
    int w = right - left;
    int h = bottom - top;

    // Convert top-left origin to bottom-left for Cocoa
    NSScreen* screen = [NSScreen mainScreen];
    CGFloat screenH = screen.frame.size.height;
    CGFloat cocoaY = screenH - top - h;

    NSRect frame = NSMakeRect(left, cocoaY, w, h);

    NSWindowStyleMask style = NSWindowStyleMaskTitled
                            | NSWindowStyleMaskClosable
                            | NSWindowStyleMaskMiniaturizable
                            | NSWindowStyleMaskResizable;

    NSWindow* nsWin = [[NSWindow alloc] initWithContentRect:frame
                                        styleMask:style
                                        backing:NSBackingStoreBuffered
                                        defer:NO];

    NSString* nsTitle = [NSString stringWithUTF8String:title];
    [nsWin setTitle:nsTitle];
    [nsWin setAcceptsMouseMovedEvents:YES];

    // Create content view
    SelfContentView* view = [[SelfContentView alloc] initWithFrame:[[nsWin contentView] bounds]];
    view.quartzWindow = this;
    [view setWantsLayer:YES];
    [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [nsWin setContentView:view];

    // Create and set delegate
    SelfWindowDelegate* delegate = [[SelfWindowDelegate alloc] init];
    delegate.quartzWindow = this;
    [nsWin setDelegate:delegate];

    // Store references
    _ns_window = (void*)nsWin;
    _ns_view = (void*)view;

    // Set up WindowRef compatibility struct
    _windowPtr.nsWindow = _ns_window;
    _windowPtr.nsView = _ns_view;
    _windowPtr.quartzWindow = this;
    _quartz_win = &_windowPtr;

    // Set up GrafPtr compatibility struct
    _grafPtr.nsView = _ns_view;
    _grafPtr.nsWindow = _ns_window;
    _grafPtr.quartzWindow = this;

    WindowSet::add_window(_quartz_win);
    _is_open = true;

    init_colors();
    init_events();

    return true;
  }
}

void QuartzWindow::init_colors() {
  CGFloat redc[]    = {1.0, 0.0, 0.0, 1.0};
  CGFloat yellowc[] = {1.0, 1.0, 0.0, 1.0};
  CGFloat blackc[]  = {0.0, 0.0, 0.0, 1.0};
  CGFloat grayc[]   = {0.5, 0.5, 0.5, 1.0};
  CGFloat whitec[]  = {1.0, 1.0, 1.0, 1.0};
  _color_space = CGColorSpaceCreateDeviceRGB();
  _red    = (long int) CGColorCreate( _color_space, redc);
  _yellow = (long int) CGColorCreate( _color_space, yellowc);
  _black  = (long int) CGColorCreate( _color_space, blackc);
  _gray   = (long int) CGColorCreate( _color_space, grayc);
  _white  = (long int) CGColorCreate( _color_space, whitec);
}


void QuartzWindow::init_font_info() {
  @autoreleasepool {
    NSString* fontName = [NSString stringWithUTF8String:default_fixed_font_name()];
    CTFontRef font = CTFontCreateWithName((__bridge CFStringRef)fontName,
                                           (CGFloat)default_fixed_font_size(), NULL);
    if (!font) {
      // Fallback to Menlo
      font = CTFontCreateWithName(CFSTR("Menlo"), (CGFloat)default_fixed_font_size(), NULL);
    }
    if (!font) fatal("could not find font");

    _default_ct_font = font;

    // Fill ATSFontMetrics from CTFont
    _metrics.version = 1;
    _metrics.ascent = (float)(CTFontGetAscent(font) / CTFontGetSize(font));
    _metrics.descent = (float)(-CTFontGetDescent(font) / CTFontGetSize(font));
    _metrics.leading = (float)(CTFontGetLeading(font) / CTFontGetSize(font));

    // Get advance width from space character
    UniChar space = ' ';
    CGGlyph glyph;
    CGSize advance;
    CTFontGetGlyphsForCharacters(font, &space, &glyph, 1);
    CTFontGetAdvancesForGlyphs(font, kCTFontOrientationHorizontal, &glyph, &advance, 1);
    _metrics.avgAdvanceWidth = (float)(advance.width / CTFontGetSize(font));
    _metrics.maxAdvanceWidth = _metrics.avgAdvanceWidth; // monospace font

    _metrics.minLeftSideBearing = 0;
    _metrics.minRightSideBearing = 0;
    _metrics.stemWidth = 0;
    _metrics.stemHeight = 0;
    _metrics.capHeight = (float)(CTFontGetCapHeight(font) / CTFontGetSize(font));
    _metrics.xHeight = (float)(CTFontGetXHeight(font) / CTFontGetSize(font));
    _metrics.italicAngle = (float)CTFontGetSlantAngle(font);
    _metrics.underlinePosition = (float)(CTFontGetUnderlinePosition(font) / CTFontGetSize(font));
    _metrics.underlineThickness = (float)(CTFontGetUnderlineThickness(font) / CTFontGetSize(font));
  }
}


void QuartzWindow::queue_bounds_changed() {
  set_bounds_changed();
  OpaqueEventRef* evt = new OpaqueEventRef();
  evt->eventClass = kEventClassWindow;
  evt->eventKind = kEventWindowBoundsChanged;
  evt->eventTime = [[NSProcessInfo processInfo] systemUptime];
  evt->setParam_ptr(kEventParamWindowRef, typeWindowRef, my_window());
  put_event(evt);
  evt->release();
}


void QuartzWindow::activate() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    SelfContentView* view = (__bridge SelfContentView*)_ns_view;
    if (![nsWin isVisible])
      [nsWin orderFront:nil];
    [nsWin makeKeyAndOrderFront:nil];
    [nsWin makeFirstResponder:view];
    // Cocoa does not send windowDidResize on first show; ui1 first paint
    // waits on boundsChanged (configureNotify). Same event as a user resize.
    queue_bounds_changed();
  }
}


void QuartzWindow::close() {
  if (!is_open())
    return;
  @autoreleasepool {
    destroyBitmapContext();
    CGColorRelease((CGColorRef) _red);
    CGColorRelease((CGColorRef) _yellow);
    CGColorRelease((CGColorRef) _black);
    CGColorRelease((CGColorRef) _gray);
    CGColorRelease((CGColorRef) _white);
    if (_color_space) CGColorSpaceRelease(_color_space);
    if (_default_ct_font) CFRelease(_default_ct_font);
    _default_ct_font = NULL;

    WindowSet::rm_window(_quartz_win);

    NSWindow* nsWin = (NSWindow*)_ns_window;
    SelfContentView* view = (SelfContentView*)_ns_view;
    view.quartzWindow = nil;
    id delegate = [nsWin delegate];
    if ([delegate isKindOfClass:[SelfWindowDelegate class]]) {
        ((SelfWindowDelegate*)delegate).quartzWindow = nil;
    }
    [nsWin setDelegate:nil];
    [nsWin close];
    _ns_window = NULL;
    _ns_view = NULL;

    _is_open = false;
    _quartz_win = NULL;
  }
}


int QuartzWindow::screen_width() {
  @autoreleasepool {
    return (int)[[NSScreen mainScreen] frame].size.width;
  }
}

int QuartzWindow::screen_height() {
  @autoreleasepool {
    return (int)[[NSScreen mainScreen] frame].size.height;
  }
}

int QuartzWindow::menubar_height() {
  return (int)[[NSApp mainMenu] menuBarHeight];
}


int QuartzWindow::left() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSRect content = [nsWin contentRectForFrameRect:[nsWin frame]];
    return (int)content.origin.x;
  }
}

int QuartzWindow::top() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSRect content = [nsWin contentRectForFrameRect:[nsWin frame]];
    CGFloat screenH = [[NSScreen mainScreen] frame].size.height;
    return (int)(screenH - content.origin.y - content.size.height);
  }
}

int QuartzWindow::width() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSRect content = [nsWin contentRectForFrameRect:[nsWin frame]];
    return (int)content.size.width;
  }
}

int QuartzWindow::height() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSRect content = [nsWin contentRectForFrameRect:[nsWin frame]];
    return (int)content.size.height;
  }
}


int QuartzWindow::inset_left() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSRect frame = [nsWin frame];
    NSRect content = [nsWin contentRectForFrameRect:frame];
    return (int)(content.origin.x - frame.origin.x);
  }
}

int QuartzWindow::inset_top() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSRect frame = [nsWin frame];
    NSRect content = [nsWin contentRectForFrameRect:frame];
    return (int)((frame.origin.y + frame.size.height) - (content.origin.y + content.size.height));
  }
}

int QuartzWindow::inset_right() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSRect frame = [nsWin frame];
    NSRect content = [nsWin contentRectForFrameRect:frame];
    return (int)((frame.origin.x + frame.size.width) - (content.origin.x + content.size.width));
  }
}

int QuartzWindow::inset_bottom() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSRect frame = [nsWin frame];
    NSRect content = [nsWin contentRectForFrameRect:frame];
    return (int)(content.origin.y - frame.origin.y);
  }
}


int QuartzWindow::font_width()  {
  return ceil(_metrics.maxAdvanceWidth * default_fixed_font_size());
}
int QuartzWindow::font_height() {
  return ceil(_metrics.leading) * default_fixed_font_size();
}

const char* QuartzWindow::default_fixed_font_name() { return "Monaco"; }
int   QuartzWindow::default_fixed_font_size() { return 9; }


bool QuartzWindow::change_extent(int left, int top, int w, int h) {
  @autoreleasepool {

    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    CGFloat screenH = [[NSScreen mainScreen] frame].size.height;

    // Convert from outer coordinates to content coordinates
    int contentLeft = left + inset_left();
    int contentTop = top + inset_top();
    int contentW = w - inset_left() - inset_right();
    int contentH = h - inset_top() - inset_bottom();

    CGFloat cocoaY = screenH - contentTop - contentH;
    NSRect contentRect = NSMakeRect(contentLeft, cocoaY, contentW, contentH);
    NSRect frameRect = [nsWin frameRectForContentRect:contentRect];
    [nsWin setFrame:frameRect display:YES];

    adjust_after_resize();
    return true;
  }
}


bool QuartzWindow::tell_platform_size_hints() {
  @autoreleasepool {
    NSWindow* nsWin = (__bridge NSWindow*)_ns_window;
    NSSize minSize, maxSize;
    minSize.width  =  _min_w == -1 ?        0  :  (_min_w - inset_left() - inset_right());
    minSize.height =  _min_h == -1 ?        0  :  (_min_h - inset_top()  - inset_bottom());
    maxSize.width  =  _max_w == -1 ?  1000000  :  (_max_w - inset_left() - inset_right());
    maxSize.height =  _max_h == -1 ?  1000000  :  (_max_h - inset_top()  - inset_bottom());
    [nsWin setContentMinSize:minSize];
    [nsWin setContentMaxSize:maxSize];
    return true;
  }
}


void QuartzWindow::setupCTM() {
  CGContextTranslateCTM(myContext, 0, height());
  CGContextScaleCTM(myContext, 1, -1);
}

void QuartzWindow::adjust_after_resize() {
  // Don't destroy bitmap here — ensureBitmapContext() will detect the size
  // mismatch and recreate on the next actual draw call. In the meantime,
  // drawRect: can blit the old bitmap (stretched to fit).
  myContext = NULL;  // force CTM re-setup on next pre_draw
  if (TheSpy != NULL)
    TheSpy->adjust_after_resize();
}


// Drawing:

bool QuartzWindow::pre_draw(bool incremental) {
  if ( get_graphics_semaphore())  return false;
  if (!_is_open) return false;
  if (_was_closed) {
    TheSpy->deactivate();
    _was_closed = false;
    return false;
  }
  if (_bounds_changed) {
    _bounds_changed = false;
    adjust_after_resize();
  }
  ensureBitmapContext();
  bool fresh = (myContext != _bitmapContext);  // detect new or recreated context
  myContext = _bitmapContext;
  if (fresh && myContext) {
    setupCTM();
    CGContextSetTextMatrix(myContext, CGAffineTransformMake( 1, 0, 0, -1, 0, 0));
    CGContextSetShouldAntialias(myContext, false);
  }
  if (fresh || !incremental) {
    clear_rectangle(0, 0, width(), height());
  }
  return true;
}


void QuartzWindow::post_draw(bool incremental) {
  @autoreleasepool {
    SelfContentView* view = (__bridge SelfContentView*)_ns_view;
    blitIOSurfaceToView(view, _ioSurface);
  }
}


void QuartzWindow::full_redraw() {
  if (TheSpy != NULL)
    TheSpy->full_redraw();
}


void QuartzWindow::draw_text(const char* text, int x, int y)  {
  int len = strlen(text);
  int h = font_height();

  clear_rectangle(x, y-h, len * font_width(), h);

  // Use Core Text instead of deprecated CGContextShowText.
  // The graphics context has a flipped CTM (y increases downward) and
  // a flipped text matrix.  Core Text expects an identity text matrix
  // and handles glyph positioning itself, so we temporarily reset it.
  @autoreleasepool {
    CFStringRef cfStr = CFStringCreateWithBytes(
        kCFAllocatorDefault, (const UInt8*)text, len,
        kCFStringEncodingMacRoman, false);
    if (!cfStr) return;
    CFStringRef keys[] = { kCTFontAttributeName };
    CFTypeRef   vals[] = { _default_ct_font };
    CFDictionaryRef attrs = CFDictionaryCreate(
        kCFAllocatorDefault, (const void**)keys, (const void**)vals,
        1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFAttributedStringRef attrStr = CFAttributedStringCreate(
        kCFAllocatorDefault, cfStr, attrs);
    CTLineRef line = CTLineCreateWithAttributedString(attrStr);

    // Core Text draws upward from the baseline in the text matrix
    // coordinate system.  With our flipped CTM, set a text matrix
    // that flips glyphs back upright and positions at (x, y).
    CGAffineTransform savedTextMatrix = CGContextGetTextMatrix(myContext);
    CGContextSetTextMatrix(myContext,
        CGAffineTransformMake(1, 0, 0, -1, x, y));
    CTLineDraw(line, myContext);
    CGContextSetTextMatrix(myContext, savedTextMatrix);

    CFRelease(line);
    CFRelease(attrStr);
    CFRelease(attrs);
    CFRelease(cfStr);
  }
}

void QuartzWindow::draw_line(int x1, int y1, int x2, int y2) {
  CGContextBeginPath(myContext);
  CGContextMoveToPoint(myContext, x1, y1);
  CGContextAddLineToPoint(myContext, x2, y2);
  CGContextStrokePath(myContext);
}

void QuartzWindow::draw_rectangle_black(int x, int y, int w, int h) {
  set_color(black());
  CGContextStrokeRectWithWidth(myContext, CGRectMake(x, y, w, h), 1.0);
}

// What Claude says about the +-1's I am getting rid of while I am working on ui1 on Quartz: (dmu, 6/26)
// 
// Who & when — not you, and not the recent ui1-on-Quartz work. The +1/-1s are original classic-Self Quartz code:
//
//   - They first appear in vm/src/any/os/quartzWindow.cpp in commit 2d10d299 — Russell Allen's 2008 "Initial commit of 4.3 CVS tree" (the repo root; the code itself is older still, from
//   the classic Mac OS X Self 4.3 Quartz layer).
//   - Russell copied them verbatim into the new vm64/src/any/os/quartzWindow.mm in cf3a27a7 (2026-02-27, "Port Quartz graphics layer to ARM64 macOS with Cocoa/CoreText"). Blame confirms
//   lines 1184–1192 are his port, untouched except for your comment-out.
//   - You commented them out in f7c8a0bd (2026-06-05). That's committed and matches the working tree — no uncommitted changes.
//
//   Precision note: only two prims ever had them — clear_rectangle (x-1, y-1, w+1, h+1) and fill_rectangle (x, y-1, w, h+1). draw_rectangle_black uses x,y,w,h straight, so there's
//   nothing to remove there.
//
//   Why they were there. The context is y-flipped (lines 1075–1076: TranslateCTM(0, height()) + ScaleCTM(1,-1), plus isFlipped → YES). Working out the pixel coverage, the fudge expands
//   the painted region by exactly one pixel on the top (and, for clear_rectangle, one on the left) — not right/bottom. They're empirical "over-erase the top/left seam" guards tuned
//   against 2008-era Quartz, where the flipped CTM left a faint 1px edge artifact. Crude pixel-alignment hacks, not load-bearing logic.
//   
//   Is it safe to comment them out? Yes — and it's the right direction.
//
//   - Mechanically safe: pure geometry on two fill calls; worst case is a 1px visual artifact, never a crash/corruption. And you kept them as /*-1*/ so they're self-documenting and
//   trivially restorable. Good way to run an experiment.
//   - Why it's correct now: you're fixing the convention properly elsewhere in the same session — 64ff82e8 ("cleaning up oldStyle rectangle") reworks the Self-level rect/point model,
//   and d0dc5cdb ("Turd prevention") replaces truncating asSmallInteger with floor (origin) / ceil (size) at the float→int boundary in quartz_wrappers.self. That floor/ceil is the
//   principled version of what the crude -1/+1 was groping at. With the convention fixed at the Self level, the VM-level fudge is redundant and now additive — the extra left/top pixel
//   becomes a turd instead of preventing one. That left-column over-clear matches the memory's residual symptom (the bodyLight on left 1px strip).
//   - The one real risk: the fudge over-erased. If any Self drawing still bleeds 1px above/left of its nominal rect, removing the expanded clear can leave the opposite turd (a stale 1px
//   strip). The call sites are full-window clear (clear_rectangle(0,0,width,height), line 1112), text-line clear (line 1136), and the Self clearRectangle/fillRectangle prims — so it's
//   exactly the drag/redraw paths you're tuning.
//
//   So: keep it commented out, but the verdict is visual, not analytic — drag a window and watch the left/top edges. Given 64ff82e8 + d0dc5cdb, I'd expect it to be cleaner now, not
//   worse. If a 1px stale strip reappears on bottom/right, that's a different edge than the fudge guarded, so the culprit is the Self-side bevel/deltaList coverage, not these two lines.


void QuartzWindow::clear_rectangle(int x, int y, int w, int h) {
  set_color(white());
  CGContextFillRect(myContext, CGRectMake(x/*-1*/, y/*-1*/, w/*+1*/, h/*+1*/));
  set_color(black());
}

void QuartzWindow::fill_rectangle(int x, int y, int w, int h) {
  CGContextFillRect(myContext, CGRectMake(x, y/*-1*/, w, h/*+1*/));
}

void QuartzWindow::set_color(long int c) {
  CGContextSetFillColorWithColor(   myContext, (CGColorRef)c );
  CGContextSetStrokeColorWithColor( myContext, (CGColorRef)c );
}

void QuartzWindow::set_thickness(int t) {
  CGContextSetLineWidth(myContext, max(1, t));
}

void QuartzWindow::set_xor()   { CGContextSetBlendMode(myContext, kCGBlendModeDifference);  }
void QuartzWindow::set_copy()  { CGContextSetBlendMode(myContext, kCGBlendModeNormal);  }


bool QuartzWindow::get_graphics_semaphore() {
  extern bool quartz_semaphore;
  return quartz_semaphore;
}


void QuartzWindow::warp_pointer(int x, int y) {
    CGPoint pt;
    pt.x = x;
    pt.y = y;
    const int n = 16;
    CGDisplayCount count = 0;
    CGDirectDisplayID dspys[n];
    CGDisplayErr err = CGGetDisplaysWithPoint( pt, n, dspys, &count);
    if (err != kCGErrorSuccess) return;
    for (uint32 i = 0;  i < count;  ++i) {
      CGRect bounds = CGDisplayBounds(dspys[i]);
      CGPoint adjusted_pt;
      adjusted_pt.x = pt.x - bounds.origin.x;
      adjusted_pt.y = pt.y - bounds.origin.y;
      CGDisplayMoveCursorToPoint( dspys[i], adjusted_pt);
    }
}


// Clipboard

oop QuartzWindow::get_scrap_text() {
  @autoreleasepool {
    NSPasteboard* pb = [NSPasteboard generalPasteboard];
    NSString* str = [pb stringForType:NSPasteboardTypeString];
    if (!str) return new_string("", 0);

    const char* utf8 = [str UTF8String];
    int len = (int)strlen(utf8);
    byteVectorOop r = Memory->byteVectorObj->cloneSize(len, CANFAIL);
    if (r->is_mark()) return new_string("", 0);
    memcpy(r->bytes(), utf8, len);
    return r;
  }
}

int QuartzWindow::put_scrap_text(const char* s, int len) {
  @autoreleasepool {
    NSPasteboard* pb = [NSPasteboard generalPasteboard];
    [pb clearContents];
    NSString* str = [[NSString alloc] initWithBytes:s length:len encoding:NSUTF8StringEncoding];
    if (!str) return -1;
    [pb setString:str forType:NSPasteboardTypeString];
    return 0;
  }
}


// Convert WindowRef to QuartzWindow
QuartzWindow* QuartzWindow::getPlatformWindow(WindowRef ww, void* FH) {
    if (!WindowSet::includes_window(ww))  { failure(FH, "not a QuartzPlatformWindow"); return NULL; }
    QuartzWindow* w = (QuartzWindow*) ww->quartzWindow;
    assert(w->my_window() == ww, "");
    return w;
}



// Events

int  QuartzWindow::events_pending(void* FH) {
  if (!is_open())  { failure(FH, "window is closed"); return 0; }
  return _evtQ.count();
}

EventRef  QuartzWindow::peek_event(void* FH) {
  if (!is_open())  { failure(FH, "window is closed"); return 0; }
  EventRef e = _evtQ.peek();
  if (e == NULL) {
    failure(FH, "no more events");
    return NULL;
  }
  return e;
}

EventRef  QuartzWindow::next_event(void* FH) {
  if (!is_open())  { failure(FH, "window is closed"); return 0; }
  EventRef e = _evtQ.get();
  if (e == NULL) {
    failure(FH, "no more events");
    return NULL;
  }
  return e;
}

void QuartzWindow::put_event(EventRef e) {
  _evtQ.put(e);
}


void QuartzWindow::init_events() {
  // On Cocoa, events are handled by the NSView/NSWindowDelegate
  // No explicit event handler installation needed
}

OSStatus QuartzWindow::AddHandledEvent_wrap( uint32* eclass, uint ec_len, uint ekind, void* FH) {
  // On Cocoa, we receive all events automatically via the view/delegate
  // This is a no-op but returns success
  if (ec_len != 1) { failure(FH, "class needs to have four bytes"); return -1; }
  return noErr;
}

OSStatus QuartzWindow::RemoveHandledEvent_wrap( uint32* eclass, uint ec_len, uint ekind, void* FH) {
  if (ec_len != 1) { failure(FH, "class needs to have four bytes"); return -1; }
  return noErr;
}


void QuartzWindow::check_carbon_events() {
  if (!cocoa_initialized) {
    return;  // No Cocoa yet, nothing to pump
  }
  if (system_is_sleeping) {
    return;  // Don't pump events during sleep/wake — display state is invalid
  }
  if (isStackOverflow((char*)currentFrame())) {
    return;  // Not enough stack for QuartzCore transaction commit
  }

  // On Cocoa, pump the event loop briefly
  @autoreleasepool {
    g_quartz_pump_count++;
    BlockGlueFlag f(quartz_semaphore);
    for (;;) {
      if (system_is_sleeping) break;
      NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                              untilDate:nil
                              inMode:NSDefaultRunLoopMode
                              dequeue:YES];
      if (!event) break;
      [NSApp sendEvent:event];
    }
    // Refresh display for all windows from their current bitmap state.
    // This provides progressive rendering during long QDBegin/QDEnd draws.
    for (int i = 0; i < WindowSet::num_windows(); i++) {
      OpaqueWindowPtr* wr = (OpaqueWindowPtr*)WindowSet::get_window(i);
      if (wr && wr->nsView && wr->quartzWindow) {
        QuartzWindow* qw = (QuartzWindow*)wr->quartzWindow;
        if (qw->bitmapContext()) {
          SelfContentView* v = (__bridge SelfContentView*)wr->nsView;
          blitIOSurfaceToView(v, qw->ioSurface());
        }
      }
    }
  }
}


// ======================================================================
// Carbon-compatible window functions (called by glue)
// ======================================================================

void SetPortWindowPort(WindowRef w) {
    // No-op on Cocoa
}

GrafPtr GetWindowPort(WindowRef w) {
    if (!w) return NULL;
    QuartzWindow* qw = (QuartzWindow*)w->quartzWindow;
    return qw->my_grafPtr();
}

int32 GetWindowPortLeft(WindowRef w) {
    if (!w) return 0;
    QuartzWindow* qw = (QuartzWindow*)w->quartzWindow;
    return 0; // port bounds start at 0
}

int32 GetWindowPortRight(WindowRef w) {
    if (!w) return 0;
    QuartzWindow* qw = (QuartzWindow*)w->quartzWindow;
    return qw->width();
}

int32 GetWindowPortTop(WindowRef w) {
    if (!w) return 0;
    return 0;
}

int32 GetWindowPortBottom(WindowRef w) {
    if (!w) return 0;
    QuartzWindow* qw = (QuartzWindow*)w->quartzWindow;
    return qw->height();
}

bool IsWindowVisible(WindowRef w) {
    if (!w) return false;
    @autoreleasepool {
        NSWindow* nsWin = (__bridge NSWindow*)w->nsWindow;
        return [nsWin isVisible];
    }
}

void ShowWindow(WindowRef w) {
    if (!w) return;
    @autoreleasepool {
        NSWindow* nsWin = (__bridge NSWindow*)w->nsWindow;
        [nsWin orderFront:nil];
    }
}

void SelectWindow(WindowRef w) {
    if (!w) return;
    @autoreleasepool {
        NSWindow* nsWin = (__bridge NSWindow*)w->nsWindow;
        [nsWin makeKeyAndOrderFront:nil];
    }
}

OSStatus ActivateWindow(WindowRef w, bool activate) {
    if (!w) return -1;
    @autoreleasepool {
        NSWindow* nsWin = (__bridge NSWindow*)w->nsWindow;
        if (activate)
            [nsWin makeKeyWindow];
        return noErr;
    }
}

OSStatus SetUserFocusWindow(WindowRef w) {
    if (!w) return -1;
    @autoreleasepool {
        NSWindow* nsWin = (__bridge NSWindow*)w->nsWindow;
        [nsWin makeKeyWindow];
        return noErr;
    }
}

void BringToFront(WindowRef w) {
    if (!w) return;
    @autoreleasepool {
        NSWindow* nsWin = (__bridge NSWindow*)w->nsWindow;
        [nsWin orderFront:nil];
    }
}

void SendBehind(WindowRef front, WindowRef behind) {
    if (!front) return;
    @autoreleasepool {
        NSWindow* nsFront = (__bridge NSWindow*)front->nsWindow;
        if (behind) {
            NSWindow* nsBehind = (__bridge NSWindow*)behind->nsWindow;
            [nsFront orderWindow:NSWindowBelow relativeTo:[nsBehind windowNumber]];
        } else {
            [nsFront orderBack:nil];
        }
    }
}

bool IsValidWindowRef(WindowRef w) {
    return w && WindowSet::includes_window(w);
}

WindowRef GetWindowFromPort(GrafPtr p) {
    if (!p) return NULL;
    QuartzWindow* qw = (QuartzWindow*)p->quartzWindow;
    return qw ? qw->my_window() : NULL;
}


// ======================================================================
// QDBeginCGContext / QDEndCGContext replacements for ARM64
// ======================================================================

CGContextRef QDBeginCGContext_wrap(OpaqueGrafPtr* port, void* FH) {
    QuartzWindow* qw = (QuartzWindow*)port->quartzWindow;
    if (!qw) { failure(FH, "no QuartzWindow"); return NULL; }
    qw->ensureBitmapContext();
    CGContextRef ctx = qw->bitmapContext();
    if (!ctx) { failure(FH, "could not get bitmap context"); return NULL; }
    CGContextSaveGState(ctx);
    // Reset CTM to identity — Self GUI code sets its own transforms
    CGAffineTransform ctm = CGContextGetCTM(ctx);
    CGContextConcatCTM(ctx, CGAffineTransformInvert(ctm));
    return ctx;
}

void QDEndCGContext_wrap(OpaqueGrafPtr* port, CGContext* carg, void* FH) {
    if (carg) CGContextRestoreGState(carg);
    QuartzWindow* qw = (QuartzWindow*)port->quartzWindow;
    @autoreleasepool {
        SelfContentView* view = (__bridge SelfContentView*)port->nsView;
        blitIOSurfaceToView(view, qw ? qw->ioSurface() : NULL);
    }
}


// Integer screen-font advances (Verdana 12 "g"=8, not CTLine's 7.477) with the
// working CTLineDraw path. CTFontDrawGlyphs of a screen font into the ui1
// offscreen painted nothing (text vanished). Per-character CTLineDraw uses the
// same text matrix as indexed; positions come from
// NSFontIntegerAdvancementsRenderingMode. -- grok 08/26/26
void DrawTextCoreText_wrap( CGContextRef ctx,
                            char* text,     uint32 textLen,
                            char* fontName, uint32 fontNameLen,
                            float size, float x, float y) {
  if ((ctx == NULL) || (text == NULL) || (fontName == NULL) || (textLen == 0))
    return;
  @autoreleasepool {
    NSString* fn = [[NSString alloc] initWithBytes:fontName
                                            length:fontNameLen
                                          encoding:NSUTF8StringEncoding];
    NSString* str = [[NSString alloc] initWithBytes:text
                                             length:textLen
                                           encoding:NSMacOSRomanStringEncoding];
    if (fn == nil || str == nil)  return;
    NSFont* outline = [NSFont fontWithName:fn size:(CGFloat)size];
    if (outline == nil)  outline = [NSFont systemFontOfSize:(CGFloat)size];
    NSFont* screen = [outline screenFontWithRenderingMode:
                        NSFontIntegerAdvancementsRenderingMode];
    if (screen == nil)  screen = outline;
    CTFontRef ctOutline = (__bridge CTFontRef)outline;
    CTFontRef ctScreen  = (__bridge CTFontRef)screen;
    CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorFromContextAttributeName };
    CFTypeRef   vals[] = { ctOutline, kCFBooleanTrue };
    CFDictionaryRef attrs = CFDictionaryCreate(NULL, (const void**)keys, (const void**)vals, 2,
                          &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    NSUInteger n = [str length];
    CGFloat pen = (CGFloat)floor((double)x + 0.5);
    CGFloat py  = (CGFloat)floor((double)y + 0.5);
    for (NSUInteger i = 0; i < n; i++) {
      unichar c = [str characterAtIndex:i];
      CFStringRef one = CFStringCreateWithCharacters(NULL, &c, 1);
      CFAttributedStringRef astr = CFAttributedStringCreate(NULL, one, attrs);
      CTLineRef line = CTLineCreateWithAttributedString(astr);
      CGContextSetTextPosition(ctx, pen, py);
      CTLineDraw(line, ctx);
      CFRelease(line);
      CFRelease(astr);
      CFRelease(one);
      CGGlyph g = 0;
      CTFontGetGlyphsForCharacters(ctScreen, &c, &g, 1);
      NSSize adv = [screen advancementForGlyph:(NSGlyph)g];
      if (adv.width <= 0)  adv.width = 1;
      pen += adv.width;
    }
    CFRelease(attrs);
  }
}


// ======================================================================
// ATSU compatibility types - destructor implementations
// ======================================================================

OpaqueATSUTextLayout::~OpaqueATSUTextLayout() {
    if (line) CFRelease(line);
    if (text) free(text);
    if (attrs) CFRelease(attrs);
}

OpaqueATSUStyle::~OpaqueATSUStyle() {
    if (font) CFRelease(font);
}


// ======================================================================
// VM console window
// ======================================================================
//
// A terminal-ish window for the VM prompt and the world's shell, used when
// Self.app is launched from the Finder and so has no real terminal.  A pty
// is spliced over fds 0/1/2: the completely unmodified REPL and Self-level
// shell read and print through it (stdin never reaches EOF, isatty(0) is
// true, and the tty line discipline provides echo and line editing).  The
// window appends the pty master's output and feeds keystrokes back into it.
// Everything runs on the VM's own (initial) thread: output is drained by a
// run-loop timer, which fires whenever anything pumps events -- the world's
// event loop, a modal panel, or the REPL's own wait loop (see
// vm_console_block_until_input in shell.cpp).

#include <util.h>       // openpty
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include <float.h>

extern bool VMConsoleActive;         // defined in shell.cpp

// Test-harness counters (see SELF_TEST_DUMP_CONSOLE below).
extern long g_quartz_pump_count;
static long g_console_key_bytes   = 0;
static long g_console_drain_bytes = 0;
long g_console_inject_sched = 0;
long g_console_inject_fired = 0;

@interface SelfVMConsoleView : NSTextView {
 @public
  int masterFD;
}
@end

@implementation SelfVMConsoleView

- (void)keyDown: (NSEvent*)e {
  if ([e modifierFlags] & NSEventModifierFlagCommand) {
    [super keyDown:e];   // menu key equivalents
    return;
  }
  NSString* chars = [e characters];
  if ([chars length] == 0) return;
  unichar u = [chars characterAtIndex:0];
  if (u >= 0xF700 && u <= 0xF8FF) return;   // arrows, function keys
  const char* bytes = [chars UTF8String];
  if (bytes) {
    g_console_key_bytes += write(masterFD, bytes, strlen(bytes));
    // The world watches stdin with O_ASYNC and sleeps until SIGIO.  A tty
    // only sends SIGIO to its foreground process group, which this pty
    // lacks when launchd made us a process-group leader (setsid/TIOCSCTTY
    // then both fail).  We know exactly when input arrives -- we just
    // wrote it -- so ring the bell ourselves.  A spurious ring is safe:
    // the world re-polls a nonblocking fd and goes back to sleep.
    raise(SIGIO);
    if (strlen(bytes) == 1) [self maybeDeliverJobControlSignal:bytes[0]];
  }
}

// ^C (and ^\) parity with a real terminal.  The line discipline recognizes
// the INTR char (it already flushed the input queue when we wrote it above)
// but its SIGINT goes to the pty's foreground process group -- nowhere,
// under a Finder launch.  Deliver it ourselves, only under the conditions
// a terminal would: ISIG enabled (raw-mode worlds read ^C as data), no
// working foreground group (else the tty just delivered it and we must not
// double-signal), and the VM's handler installed (raising SIGINT before
// SignalInterface::initialize would terminate the process, its default).
- (void)maybeDeliverJobControlSignal: (char)c {
  struct termios tio;
  if (tcgetattr(0, &tio) != 0) return;
  if (!(tio.c_lflag & ISIG)) return;
  if (tcgetpgrp(0) > 0) return;
  int sig;
  if      (tio.c_cc[VINTR] != _POSIX_VDISABLE && c == (char)tio.c_cc[VINTR]) sig = SIGINT;
  else if (tio.c_cc[VQUIT] != _POSIX_VDISABLE && c == (char)tio.c_cc[VQUIT]) sig = SIGQUIT;
  else return;
  struct sigaction cur;
  if (sigaction(sig, NULL, &cur) != 0) return;
  if (cur.sa_handler == SIG_DFL || cur.sa_handler == SIG_IGN) return;
  raise(sig);
}

- (void)paste: (id)sender {
  NSString* s = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
  if (!s) return;
  const char* bytes = [s UTF8String];
  if (bytes) {
    write(masterFD, bytes, strlen(bytes));
    raise(SIGIO);   // see keyDown
  }
}

// The view is editable only so that it draws a caret.  Keystrokes never
// reach the text system (keyDown above writes them to the pty), but drops
// and key-binding actions would; refuse them all.  appendOutput: edits the
// text storage directly and is not consulted here.
- (BOOL)shouldChangeTextInRange: (NSRange)r replacementString: (NSString*)s {
  return NO;
}

@end

@interface SelfVMConsole : NSObject <NSWindowDelegate> {
 @public
  NSWindow*          window;
  SelfVMConsoleView* view;
  int                masterFD;
  BOOL               inEscape;    // swallowing an ANSI escape sequence
}
@end

@implementation SelfVMConsole

- (id)initWithMasterFD: (int)fd {
  if (self = [super init]) {
    masterFD = fd;
    [self buildWindow];
    [self buildMenu];
    // Drain in every common run-loop mode so output appears during modal
    // panels (the snapshot chooser) as well as normal event pumping.
    NSTimer* t = [NSTimer timerWithTimeInterval:0.05
                                         target:self
                                       selector:@selector(drain:)
                                       userInfo:nil
                                        repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:t forMode:NSRunLoopCommonModes];

    [self dumpForTestHarness];   // baseline dump before any timer fires

    // Test harness: with SELF_TEST_TYPE=<text> set, feed <text> through the
    // console's keyDown path 6 seconds in (after the world has booted), as
    // if typed.  SELF_TEST_TYPE_LATE=<text> types 14 seconds in, to test
    // interaction sequences (e.g. ^C, then answering the interrupt menu).
    // Exercises keystroke -> pty -> world -> output -> display.
    [self scheduleTestTyping:getenv("SELF_TEST_TYPE")      after:6.0];
    [self scheduleTestTyping:getenv("SELF_TEST_TYPE_LATE") after:14.0];
  }
  return self;
}

- (void)scheduleTestTyping: (const char*)toType after: (double)seconds {
  if (!toType) return;
  NSString* text = [NSString stringWithUTF8String:toType];
  extern long g_console_inject_sched, g_console_inject_fired;
  g_console_inject_sched++;
  NSTimer* it = [NSTimer timerWithTimeInterval:seconds
                                       repeats:NO
                                         block:^(NSTimer* tt){
    g_console_inject_fired++;
    for (NSUInteger i = 0; i < [text length]; i++) {
      NSString* ch = [text substringWithRange:NSMakeRange(i, 1)];
      NSEvent* ev = [NSEvent keyEventWithType:NSEventTypeKeyDown
                                     location:NSZeroPoint
                                modifierFlags:0
                                    timestamp:0
                                 windowNumber:[window windowNumber]
                                      context:nil
                                   characters:ch
                  charactersIgnoringModifiers:ch
                                    isARepeat:NO
                                      keyCode:0];
      [view keyDown:ev];
    }
  }];
  [[NSRunLoop currentRunLoop] addTimer:it forMode:NSRunLoopCommonModes];
}

- (void)buildWindow {
  NSRect frame = NSMakeRect(120, 140, 720, 440);
  window = [[NSWindow alloc]
      initWithContentRect:frame
                styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                           NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [window setTitle:@"Self Console"];
  [window setReleasedWhenClosed:NO];
  [window setDelegate:self];

  NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:[[window contentView] bounds]];
  [scroll setHasVerticalScroller:YES];
  [scroll setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];

  NSSize contentSize = [scroll contentSize];
  view = [[SelfVMConsoleView alloc] initWithFrame:NSMakeRect(0, 0, contentSize.width, contentSize.height)];
  view->masterFD = masterFD;
  // Editable, or NSTextView never draws its insertion point.  Typing still
  // goes through keyDown to the pty: shouldChangeTextInRange: refuses every
  // edit the text system itself would make.
  [view setEditable:YES];
  [view setSelectable:YES];
  [view setBackgroundColor:[NSColor textBackgroundColor]];
  [view setMinSize:NSMakeSize(0, contentSize.height)];
  [view setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
  [view setVerticallyResizable:YES];
  [view setHorizontallyResizable:NO];
  [view setAutoresizingMask:NSViewWidthSizable];
  [[view textContainer] setContainerSize:NSMakeSize(contentSize.width, FLT_MAX)];
  [[view textContainer] setWidthTracksTextView:YES];

  [scroll setDocumentView:view];
  [[window contentView] addSubview:scroll];
  [window makeFirstResponder:view];
  [window makeKeyAndOrderFront:nil];
}

- (void)buildMenu {
  // Replaces the empty menu set in ensure_cocoa_initialized.
  NSMenu* mainMenu = [[NSMenu alloc] init];

  NSMenuItem* appItem = [mainMenu addItemWithTitle:@"Self" action:nil keyEquivalent:@""];
  NSMenu* appMenu = [[NSMenu alloc] init];
  NSMenuItem* show = [appMenu addItemWithTitle:@"Show Console"
                                        action:@selector(show:)
                                 keyEquivalent:@"k"];
  [show setTarget:self];
  [appMenu addItem:[NSMenuItem separatorItem]];
  [appMenu addItemWithTitle:@"Quit Self" action:@selector(terminate:) keyEquivalent:@"q"];
  [appItem setSubmenu:appMenu];

  NSMenuItem* editItem = [mainMenu addItemWithTitle:@"Edit" action:nil keyEquivalent:@""];
  NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
  [editMenu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
  [editMenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
  [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
  [editItem setSubmenu:editMenu];

  [NSApp setMainMenu:mainMenu];
}

- (void)show: (id)sender {
  [window makeKeyAndOrderFront:sender];
}

- (BOOL)windowShouldClose: (NSWindow*)w {
  [window orderOut:nil];   // hide; reopen via Self > Show Console.  Closing
  return NO;               // for real would EOF the pty and kill the REPL.
}

- (void)drain: (NSTimer*)t {
  char buf[4096];
  for (;;) {
    ssize_t n = read(masterFD, buf, sizeof(buf));
    if (n <= 0) break;
    g_console_drain_bytes += n;
    [self appendOutput:buf length:(int)n];
  }
  [self dumpForTestHarness];
}

// Test harness: with SELF_TEST_DUMP_CONSOLE=<path> set, keep writing the
// link counters and the console's current text to <path>, so scripts can
// observe what the console shows without a screen.  Inert otherwise.
- (void)dumpForTestHarness {
  static const char* dumpPath = getenv("SELF_TEST_DUMP_CONSOLE");
  if (!dumpPath) return;
  FILE* f = fopen(dumpPath, "w");
  if (!f) return;
  fprintf(f, "pumps=%ld keyBytes=%ld drainBytes=%ld sched=%ld fired=%ld keyWindow=%d caret=%d responder=%s\n",
          g_quartz_pump_count, g_console_key_bytes, g_console_drain_bytes,
          g_console_inject_sched, g_console_inject_fired,
          (int)[window isKeyWindow],
          (int)[view shouldDrawInsertionPoint],
          [[[[window firstResponder] class] description] UTF8String]);

  // State of fd 0 (the pty slave): what mode has the world put it in,
  // and is typed input sitting in it unread?
  struct termios tio;
  if (tcgetattr(0, &tio) == 0) {
    fprintf(f, "fd0 termios: ICANON=%d ECHO=%d ICRNL=%d\n",
            (int)!!(tio.c_lflag & ICANON),
            (int)!!(tio.c_lflag & ECHO),
            (int)!!(tio.c_iflag & ICRNL));
  } else {
    fprintf(f, "fd0 termios: tcgetattr failed errno=%d\n", errno);
  }
  int fl = fcntl(0, F_GETFL, 0);
  fprintf(f, "fd0 flags: O_ASYNC=%d O_NONBLOCK=%d owner=%d pid=%d pgrp=%d tcgetpgrp=%d\n",
          fl < 0 ? -1 : (int)!!(fl & O_ASYNC),
          fl < 0 ? -1 : (int)!!(fl & O_NONBLOCK),
          fcntl(0, F_GETOWN, 0), (int)getpid(), (int)getpgrp(),
          (int)tcgetpgrp(0));
  int pending = -1;
  ioctl(0, FIONREAD, &pending);
  fprintf(f, "fd0 pending bytes: %d\n", pending);

  const char* text = [[view string] UTF8String];
  if (text) fputs(text, f);
  fclose(f);
}

- (void)appendOutput: (const char*)buf length: (int)n {
  static NSDictionary* attrs = nil;
  if (!attrs) {
    attrs = [[NSDictionary alloc] initWithObjectsAndKeys:
        [NSFont monospacedSystemFontOfSize:12.0 weight:NSFontWeightRegular],
        NSFontAttributeName,
        [NSColor textColor], NSForegroundColorAttributeName,
        nil];
  }
  NSTextStorage* ts = [view textStorage];
  NSMutableData* run = [NSMutableData data];

# define FLUSH_RUN                                                            \
    if ([run length]) {                                                       \
      NSString* s = [[[NSString alloc] initWithData:run                       \
                       encoding:NSUTF8StringEncoding] autorelease];           \
      if (!s) s = [[[NSString alloc] initWithData:run                         \
                     encoding:NSISOLatin1StringEncoding] autorelease];        \
      if (s) [ts appendAttributedString:                                      \
               [[[NSAttributedString alloc] initWithString:s                  \
                                                attributes:attrs] autorelease]]; \
      [run setLength:0];                                                      \
    }

  [ts beginEditing];
  for (int i = 0; i < n; i++) {
    unsigned char b = buf[i];
    if (inEscape)    { if (isalpha(b)) inEscape = NO;  continue; }
    if (b == 0x1B)   { FLUSH_RUN; inEscape = YES;      continue; }
    if (b == '\r')   { FLUSH_RUN;                      continue; }
    if (b == '\b')   {                    // ECHOE erase: "\b \b" nets one delete
      FLUSH_RUN;
      if ([ts length])
        [ts deleteCharactersInRange:NSMakeRange([ts length] - 1, 1)];
      continue;
    }
    [run appendBytes:&b length:1];
  }
  FLUSH_RUN
# undef FLUSH_RUN
  [ts endEditing];
  // Keep the caret at the end, where the cursor of a terminal would be --
  // unless the user has selected text (for Copy), which output must not
  // clobber.
  if ([view selectedRange].length == 0)
    [view setSelectedRange:NSMakeRange([ts length], 0)];
  [view scrollRangeToVisible:NSMakeRange([ts length], 0)];
}

@end

static SelfVMConsole* TheVMConsole = nil;

void osx_open_vm_console() {
  int master, slave;
  if (openpty(&master, &slave, NULL, NULL, NULL) != 0)
    return;   // no console; run_the_VM's park loop is the fallback

  // Set the window size before the pty has a foreground process group:
  // once it has one (below), TIOCSWINSZ raises SIGWINCH at it, and this
  // early in main a signal is at best noise.
  struct winsize ws;
  memset(&ws, 0, sizeof(ws));
  ws.ws_row = 24;
  ws.ws_col = 80;
  ioctl(master, TIOCSWINSZ, &ws);

  // Make the pty our controlling terminal, with us as its foreground
  // process group.  BSD ttys deliver async-I/O wakeups (SIGIO) to the
  // tty's foreground process group; without this there is no foreground
  // group, so the world's O_ASYNC stdin never rings and typed lines sit
  // unread in the slave forever.  (Same dance as the 2012 Cocoa console's
  // Pty.m.)  Failures are tolerated: which of setsid/TIOCSCTTY applies
  // depends on how we were launched.
  //
  // SIGTTOU/SIGTTIN must be ignored during the takeover: tcsetpgrp from a
  // not-yet-foreground group raises SIGTTOU, which loops forever in the
  // VM's signal forwarding -- and ignoring it also makes the kernel permit
  // the call.  Save/restore via sigaction to preserve handler flags.
  struct sigaction ign, oldTTOU, oldTTIN;
  memset(&ign, 0, sizeof(ign));
  ign.sa_handler = SIG_IGN;
  sigaction(SIGTTOU, &ign, &oldTTOU);
  sigaction(SIGTTIN, &ign, &oldTTIN);

  setsid();
  ioctl(slave, TIOCSCTTY, 0);
  tcsetpgrp(slave, getpgrp());

  sigaction(SIGTTOU, &oldTTOU, NULL);
  sigaction(SIGTTIN, &oldTTIN, NULL);

  lprintf("Self: opening console window\n");   // last words to the old stderr
  fflush(stdout);
  fflush(stderr);
  dup2(slave, 0);
  dup2(slave, 1);
  dup2(slave, 2);
  if (slave > 2) close(slave);
  // stdout picked its buffering while aimed at /dev/null; unbuffer so
  // output reaches the console as it is printed.
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  fcntl(master, F_SETFL, O_NONBLOCK);

  TheVMConsole = [[SelfVMConsole alloc] initWithMasterFD:master];
  VMConsoleActive = true;
}


// ======================================================================
// Finder launch: choose a world when Self.app is opened by double-click
// ======================================================================
//
// When launched from the Finder there is no terminal and no -s argument.
// Before the universe reads WorldName, pick the world from (in order):
//   1. a snapshot delivered by the Open Documents Apple Event
//      (a snapshot dropped on the icon, or a double-clicked .snap64),
//   2. a snapshot shipped in the bundle's Contents/Resources,
//   3. an open panel; cancelling it quits.
// Also sets RunningWithoutConsole so the REPL in shell.cpp parks instead
// of reading EOF from /dev/null and shutting the VM down.

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

extern bool RunningWithoutConsole;   // defined in shell.cpp
extern char* WorldName;              // universe.hh
extern char* startUpSelfFile;        // shell.hh

// Four-char Apple Event codes, spelled locally so we do not depend on
// which Carbon-era headers happen to be visible in this file.
enum {
  selfAE_coreClass = 'aevt',   // kCoreEventClass
  selfAE_openApp   = 'oapp',   // kAEOpenApplication
  selfAE_openDocs  = 'odoc',   // kAEOpenDocuments
  selfAE_keyDirect = '----',   // keyDirectObject
};

@interface SelfLaunchEventCollector : NSObject {
 @public
  BOOL            sawOpenApp;
  BOOL            launchPhaseOver;
  NSMutableArray* files;       // paths from odoc, launch phase only
}
@end

@implementation SelfLaunchEventCollector

- (id)init {
  if (self = [super init]) {
    files = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)handleOpenApp: (NSAppleEventDescriptor*)event
       withReplyEvent: (NSAppleEventDescriptor*)reply {
  sawOpenApp = YES;
}

- (void)handleOpenDocs: (NSAppleEventDescriptor*)event
        withReplyEvent: (NSAppleEventDescriptor*)reply {
  if (launchPhaseOver) return;  // a drop on the running app; one VM, one world
  NSAppleEventDescriptor* list = [event paramDescriptorForKeyword:selfAE_keyDirect];
  for (NSInteger i = 1; i <= [list numberOfItems]; i++) {
    NSURL* url = [[list descriptorAtIndex:i] fileURLValue];
    if (url) [files addObject:[url path]];
  }
}

@end

static NSString* snapshotInResources() {
  NSBundle* bundle = [NSBundle mainBundle];
  for (NSString* ext in @[@"snap64", @"snap"]) {
    NSArray* found = [bundle pathsForResourcesOfType:ext inDirectory:nil];
    if ([found count] == 0) continue;
    for (NSString* p in found)  // prefer a snapshot literally named Self
      if ([[[p lastPathComponent] stringByDeletingPathExtension]
            isEqualToString:@"Self"]) return p;
    return [found objectAtIndex:0];
  }
  return nil;
}

static bool isSnapshotPath(NSString* p) {
  return [p hasSuffix:@".snap64"] || [p hasSuffix:@".snap"];
}

void osx_choose_world_for_finder_launch() {
  // Only take over when actually launched by LaunchServices: inside an
  // .app bundle, parented by launchd, stdin on /dev/null.  A plain
  // command-line or CI invocation (tty, pipe or file on stdin, shell as
  // parent) keeps the classic behavior.  SELF_BUNDLE_LAUNCH=1 forces the
  // Finder path for testing from a terminal.
  if (getenv("SELF_BUNDLE_LAUNCH") == NULL) {
    if (getppid() != 1) return;
    struct stat s0, sn;
    if (fstat(0, &s0) != 0 || stat("/dev/null", &sn) != 0) return;
    if (s0.st_dev != sn.st_dev || s0.st_ino != sn.st_ino) return;
    if (![[[NSBundle mainBundle] bundlePath] hasSuffix:@".app"]) return;
  }

  RunningWithoutConsole = true;

  @autoreleasepool {
    ensure_cocoa_initialized();
    osx_open_vm_console();   // REPL and world shell live here from now on
  }

  if (WorldName != NULL || startUpSelfFile != NULL)
    return;  // explicit args, e.g. via `open Self.app --args -s foo.snap64`

  @autoreleasepool {
    // Install our handlers after finishLaunching so they replace, rather
    // than get replaced by, NSApplication's defaults.  The launch Apple
    // Event stays queued until pumped, so nothing is lost.
    SelfLaunchEventCollector* collector = [[SelfLaunchEventCollector alloc] init];
    NSAppleEventManager* aem = [NSAppleEventManager sharedAppleEventManager];
    [aem setEventHandler:collector
             andSelector:@selector(handleOpenDocs:withReplyEvent:)
           forEventClass:selfAE_coreClass
              andEventID:selfAE_openDocs];
    [aem setEventHandler:collector
             andSelector:@selector(handleOpenApp:withReplyEvent:)
           forEventClass:selfAE_coreClass
              andEventID:selfAE_openApp];

    // LaunchServices sends exactly one of oapp/odoc right after launch;
    // pump until it arrives (bounded, in case we were exec'd directly).
    NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:2.0];
    while (!collector->sawOpenApp && [collector->files count] == 0
           && [deadline timeIntervalSinceNow] > 0) {
      NSEvent* e = [NSApp nextEventMatchingMask:NSEventMaskAny
                                      untilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:YES];
      if (e) [NSApp sendEvent:e];
    }
    collector->launchPhaseOver = YES;

    NSString* world  = nil;
    NSString* script = nil;
    for (NSString* f in collector->files) {
      if      (!world  && isSnapshotPath(f))      world  = f;
      else if (!script && [f hasSuffix:@".self"]) script = f;
    }

    bool worldFromResources = false;
    if (!world && !script) {
      world = snapshotInResources();
      worldFromResources = world != nil;
    }

    if (!world && !script) {
#     pragma clang diagnostic push
#     pragma clang diagnostic ignored "-Wdeprecated-declarations"
      [NSApp activateIgnoringOtherApps:YES];
      NSOpenPanel* panel = [NSOpenPanel openPanel];
      [panel setMessage:@"Choose a Self snapshot (or script) to start"];
      [panel setPrompt:@"Start"];
      [panel setCanChooseFiles:YES];
      [panel setCanChooseDirectories:NO];
      [panel setAllowsMultipleSelection:NO];
      [panel setAllowedFileTypes:@[@"snap64", @"snap", @"self"]];
#     pragma clang diagnostic pop
      if (getenv("SELF_TEST_CANCEL_CHOOSER") != NULL) {
        // test hook: auto-cancel the panel shortly after it opens
        NSTimer* t = [NSTimer timerWithTimeInterval:1.5
                                            repeats:NO
                                              block:^(NSTimer* tt){ [panel cancel:nil]; }];
        [[NSRunLoop currentRunLoop] addTimer:t forMode:NSRunLoopCommonModes];
      }
      if ([panel runModal] == NSModalResponseOK) {
        NSString* f = [[[panel URLs] objectAtIndex:0] path];
        if ([f hasSuffix:@".self"]) script = f; else world = f;
      }
      // else: nothing chosen -- fall through with no world, and the REPL
      // presents the bare VM# prompt in the console window.
      // (If quitting here instead, use ::exit, NOT OS::terminate:
      // init_globals() has not run yet, so exit_globals() would tear down
      // subsystems that were never set up and hang.)
    }

    if (world)  WorldName       = strdup([world  fileSystemRepresentation]);
    if (script) startUpSelfFile = strdup([script fileSystemRepresentation]);

    // launchd starts us with cwd "/": neither writable (snapshot saves)
    // nor where the world's relative paths point.  Move somewhere sane.
    NSString* dir = ((world || script) && !worldFromResources)
      ? [(world ? world : script) stringByDeletingLastPathComponent]
      : NSHomeDirectory();
    if (dir) ::chdir([dir fileSystemRepresentation]);

    if (world)       lprintf("Self: starting world %s\n", WorldName);
    if (script)      lprintf("Self: startup script %s\n", startUpSelfFile);
    if (!world && !script)
      lprintf("Self: no world chosen; starting the bare VM\n");
  }
}


# endif // QUARTZ_LIB && __aarch64__
