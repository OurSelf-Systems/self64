/* Sun-$Revision: 30.8 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "debug.hh"
# include "_debug.cpp.incl"

# define DefineFlags(                                                         \
    flagName, flagType, flagTypeName, primReturnType,                         \
    initialValue, getCurrentValue, checkNewValue, setNewValue,                \
    explanation, wizardOnly)                                                  \
                                                                              \
    flagType flagName = initialValue;

FOR_ALL_DEBUG_PRIMS(DefineFlags)



void debug_init() {
  // initialize debug flags that use function calls
  DirPath = copy_c_heap_string("");   // so we can use free() when changing it
  SpyDisplay = copy_c_heap_string("");
  SpyFont = copy_c_heap_string("");

  // Warn about VM debug flags that are currently on. These flags slow the VM,
  // perturb timing, or force extra invariant checks — handy when you want
  // them, surprising when you don't. Reports only flags whose runtime value
  // is true at this point, so a clean run prints nothing. Flags can be flipped
  // later from Self code; this only catches the initial state.
  // -- claude & dmu May 2026
  bool any = false;
# define WARN_IF_ON(f) \
    do { if (f) { \
      if (!any) { fprintf(stderr, "[VM] active debug flags:"); any = true; } \
      fprintf(stderr, " " #f); \
    } } while (0)
  WARN_IF_ON(SpendTimeForDebugging);
  WARN_IF_ON(CheckAssertions);
  WARN_IF_ON(ZapResourceArea);
  WARN_IF_ON(VerifyBeforeScavenge);
  WARN_IF_ON(VerifyAfterScavenge);
  WARN_IF_ON(VerifyBeforeGC);
  WARN_IF_ON(VerifyAfterGC);
  WARN_IF_ON(VerifyBeforeConversion);
  WARN_IF_ON(VerifyAfterConversion);
  WARN_IF_ON(VerifyAfterRecompilation);
  WARN_IF_ON(VerifyZoneOften);
  WARN_IF_ON(ScavengeAfterRecompilation);
  WARN_IF_ON(PrintScavenge);
  WARN_IF_ON(PrintGC);
  WARN_IF_ON(ForceFrequentScavengesViaSmallNewSpace);
  WARN_IF_ON(traceP);
  WARN_IF_ON(traceV);
# undef WARN_IF_ON
  if (any) fprintf(stderr, "\n");
}

# undef DefineFlags



# if  GENERATE_DEBUGGING_AIDS

extern "C" void start_gdb_debugging() {
  // set up stuff for debugging a crashed process (call this after attach)
  // turn off timers etc. to prevent gdb crashes
  SignalBlocker sb(SignalBlocker::allow_user_int);
  static bool gdb_debugging_called = false;
  if (gdb_debugging_called) return;     // prevent multiple calls
  gdb_debugging_called = true;
  IntervalTimer::dont_use_any_timer = true;
  IntervalTimer::disable_all(true);
  TheSpy->deactivate();
  WizardMode = PrintOopAddress = true;
  flush_logFile();
}

extern "C" void redirect_stdio(char* filename) {
  // for debugging; redirect input/output to gdb window
  start_gdb_debugging();
  char devname[20];
  char* fn;
  static FILE* res;
  if (strlen(filename) == 2) {
    // shortcut: name of pseudo-tty from ps, e.g "p0" == "/dev/ttyp0"
    sprintf(devname, "/dev/tty%s", filename);
    fn = devname;
  } else {
    fn = filename;
  }
  res = freopen (fn, "a+", stdout);
  if (res) {
    lprintf("stdout redirected to %s.\n", fn);
  } else {
    lprintf("stdout couldn't be redirected to %s.\n", fn);
    perror("redirection failed");
  }
  res = freopen (fn, "a+", stderr);
  if (res) {
    lprintf("stderr redirected to %s.\n", fn);
  } else {
    lprintf("stderr couldn't be redirected to %s.\n", fn);
    perror("redirection failed");
  }
  res = freopen (fn, "r", stdin);
  if (res) {
    lprintf("stdin redirected to %s.\n", fn);
  } else {
    lprintf("stdin couldn't be redirected to %s.\n", fn);
    perror("redirection failed");
  }
  fflush(stderr);
}

#endif
