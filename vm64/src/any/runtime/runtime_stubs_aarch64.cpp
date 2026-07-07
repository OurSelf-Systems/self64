/* Copyright 2024-2026 AUTHORS.
   See the LICENSE file for license information. */

/* C/asm stubs for aarch64 (ARM64) interpreter-only builds.
   Replaces i386/runtime/runtime_asm_gcc_i386.S
   Modeled after runtime_stubs_x86_64.cpp */

# include "config.hh"
# include <cstdint>   // uintptr_t (gcc does not provide it transitively)
# if TARGET_ARCH == AARCH64_ARCH

// Mach-O prepends '_' to C symbols at the object level; ELF does not.
// Asm references to C functions must spell the object-level name.
# ifdef __APPLE__
#   define C_SYM(name) "_" name
# else
#   define C_SYM(name) name
# endif

# include "_runtime_aarch64.cpp.incl"


// =====================================================================
// Frame pointer access
// =====================================================================

extern "C" frame* currentFrame() {
  // Return the caller's frame pointer.
  // On ARM64, x29 is the frame pointer.
  // We dereference fp once to get past our own frame setup.
  frame* fp;
  __asm__ __volatile__("ldr %0, [x29]" : "=r"(fp));
  return fp;
}

extern "C" char* currentReturnAddr() {
  return (char*)__builtin_return_address(0);
}


// =====================================================================
// Stack switching (SwitchStack)
// =====================================================================
// These switch to a new stack (the VM stack) and call a function there.
// They must be implemented in assembly because they change sp.
//
// ARM64 ABI: args in x0-x7, return in x0, callee-saved x19-x28/fp/lr
// Stack must be 16-byte aligned before any call (bl/blr).

// SwitchStack0: call fn_start on newSP, no extra args
extern "C" char* SwitchStack0(char* fn_start, char* newSP) {
  // fn_start in x0, newSP in x1
  char* result;
  __asm__ __volatile__(
    "mov   x9, sp\n\t"              // save old SP
    "mov   sp, %2\n\t"              // switch to new stack
    "stp   x9, xzr, [sp, #-16]!\n\t" // push old SP + padding (16-byte aligned)
    "blr   %1\n\t"                  // call fn_start()
    "mov   %0, x0\n\t"             // capture return value from x0
    "ldp   x9, xzr, [sp], #16\n\t" // pop old SP + padding
    "mov   sp, x9\n\t"             // restore old SP
    : "=r"(result)
    : "r"(fn_start), "r"(newSP)
    : "x9", "x10", "x11", "x12", "x13", "x14", "x15",
      "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8",
      "x30", "memory", "cc"
  );
  return result;
}

extern "C" char* SwitchStack1(char* fn_start, char* newSP, void* arg1) {
  char* result;
  __asm__ __volatile__(
    "mov   x9, sp\n\t"
    "mov   sp, %2\n\t"
    "stp   x9, xzr, [sp, #-16]!\n\t"
    "mov   x0, %3\n\t"             // arg1 in x0
    "blr   %1\n\t"
    "mov   %0, x0\n\t"             // capture return value from x0
    "ldp   x9, xzr, [sp], #16\n\t"
    "mov   sp, x9\n\t"
    : "=r"(result)
    : "r"(fn_start), "r"(newSP), "r"(arg1)
    : "x9", "x10", "x11", "x12", "x13", "x14", "x15",
      "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8",
      "x30", "memory", "cc"
  );
  return result;
}

extern "C" char* SwitchStack2(char* fn_start, char* newSP, void* arg1, void* arg2) {
  char* result;
  __asm__ __volatile__(
    "mov   x9, sp\n\t"
    "mov   sp, %2\n\t"
    "stp   x9, xzr, [sp, #-16]!\n\t"
    "mov   x0, %3\n\t"
    "mov   x1, %4\n\t"
    "blr   %1\n\t"
    "mov   %0, x0\n\t"             // capture return value from x0
    "ldp   x9, xzr, [sp], #16\n\t"
    "mov   sp, x9\n\t"
    : "=r"(result)
    : "r"(fn_start), "r"(newSP), "r"(arg1), "r"(arg2)
    : "x9", "x10", "x11", "x12", "x13", "x14", "x15",
      "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8",
      "x30", "memory", "cc"
  );
  return result;
}

extern "C" char* SwitchStack3(char* fn_start, char* newSP,
                               void* arg1, void* arg2, void* arg3) {
  char* result;
  __asm__ __volatile__(
    "mov   x9, sp\n\t"
    "mov   sp, %2\n\t"
    "stp   x9, xzr, [sp, #-16]!\n\t"
    "mov   x0, %3\n\t"
    "mov   x1, %4\n\t"
    "mov   x2, %5\n\t"
    "blr   %1\n\t"
    "mov   %0, x0\n\t"             // capture return value from x0
    "ldp   x9, xzr, [sp], #16\n\t"
    "mov   sp, x9\n\t"
    : "=r"(result)
    : "r"(fn_start), "r"(newSP), "r"(arg1), "r"(arg2), "r"(arg3)
    : "x9", "x10", "x11", "x12", "x13", "x14", "x15",
      "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8",
      "x30", "memory", "cc"
  );
  return result;
}

extern "C" char* SwitchStack4(char* fn_start, char* newSP,
                               void* arg1, void* arg2,
                               void* arg3, void* arg4) {
  char* result;
  __asm__ __volatile__(
    "mov   x9, sp\n\t"
    "mov   sp, %2\n\t"
    "stp   x9, xzr, [sp, #-16]!\n\t"
    "mov   x0, %3\n\t"
    "mov   x1, %4\n\t"
    "mov   x2, %5\n\t"
    "mov   x3, %6\n\t"
    "blr   %1\n\t"
    "mov   %0, x0\n\t"             // capture return value from x0
    "ldp   x9, xzr, [sp], #16\n\t"
    "mov   sp, x9\n\t"
    : "=r"(result)
    : "r"(fn_start), "r"(newSP), "r"(arg1), "r"(arg2),
      "r"(arg3), "r"(arg4)
    : "x9", "x10", "x11", "x12", "x13", "x14", "x15",
      "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8",
      "x30", "memory", "cc"
  );
  return result;
}


// =====================================================================
// Process switching (SetSPAndCall)
// =====================================================================
// This is the heart of cooperative multitasking in Self.
// Saves/restores ALL callee-saved registers for process switching.
//
// Args: x0=callerSaveAddr, x1=calleeSaveAddr, w2=init,
//       x3=semaphore, w4=pcWasSet
//
// callerSaveAddr[0]/[1] = saved sp / saved return addr
// calleeSaveAddr[0]/[1] = saved sp / saved return addr (or new sp / entry)
//
// The saved sp (= suspendedSP) must be usable as a frame* by frame-walking
// code (Stack::first_VM_frame, etc.), so it must be:
//   - 16-byte aligned (frame::is_aligned())
//   - at *(suspendedSP+0): saved fp (used by sender())
//   - at *(suspendedSP+8): return addr = 0 (marks it as non-Self frame)
//
// Saved stack layout (at callerSaveAddr[0]):
//   [saved x19] [saved x20]
//   [saved x21] [saved x22]
//   [saved x23] [saved x24]
//   [saved x25] [saved x26]
//   [saved x27] [saved x28]
//   [saved fp]  [saved lr]
//   [copy of fp] [mock return addr=0]  <- callerSaveAddr[0] (0 mod 16)

extern "C" void ReturnOffTopOfProcess();

extern "C" __attribute__((naked))
void SetSPAndCall(char** callerSaveAddr, char** calleeSaveAddr,
                  bool init, bool8* semaphore, bool8 pcWasSet) {
  __asm__ __volatile__(
    // Prologue: save all callee-saved registers
    "stp   x19, x20, [sp, #-16]!\n\t"
    "stp   x21, x22, [sp, #-16]!\n\t"
    "stp   x23, x24, [sp, #-16]!\n\t"
    "stp   x25, x26, [sp, #-16]!\n\t"
    "stp   x27, x28, [sp, #-16]!\n\t"
    "stp   x29, x30, [sp, #-16]!\n\t"   // save fp (x29) and lr (x30)

    // Create frame-like footer for external frame walkers
    // (so suspendedSP can be interpreted as a frame* by Stack::first_VM_frame)
    // offset 0 = saved fp (for sender()), offset 8 = 0 (marks non-Self frame)
    "stp   x29, xzr, [sp, #-16]!\n\t"   // fp copy + mock return addr = 0
    // sp is now 0 mod 16

    // Save caller state if callerSaveAddr (x0) != NULL
    "cbz   x0, 1f\n\t"
    "mov   x9, sp\n\t"
    "str   x9, [x0]\n\t"                 // callerSaveAddr[0] = sp (frame-like)
    "str   x30, [x0, #8]\n\t"            // callerSaveAddr[1] = lr (return addr)
    "1:\n\t"

    // Clear semaphore
    "strb  wzr, [x3]\n\t"

    // Load callee's saved state (read before switching stacks)
    "ldr   x9, [x1]\n\t"                 // saved SP (or new SP if init)
    "ldr   x10, [x1, #8]\n\t"            // saved PC (or entry point if init)

    "tst   w2, #0xff\n\t"                // init?
    "b.ne  3f\n\t"

    // --- Resume existing process ---
    "tst   w4, #0xff\n\t"                // pcWasSet?
    "b.ne  2f\n\t"

    // Normal resume: suspendedSP is a genuine save record made by the
    // prologue above; pop it and jump to the saved PC.
    "mov   sp, x9\n\t"                   // switch to callee's saved stack
    "add   sp, sp, #16\n\t"              // skip footer (fp copy + mock ret)
    "ldp   x29, x30, [sp], #16\n\t"      // restore fp and lr
    "ldp   x27, x28, [sp], #16\n\t"
    "ldp   x25, x26, [sp], #16\n\t"
    "ldp   x23, x24, [sp], #16\n\t"
    "ldp   x21, x22, [sp], #16\n\t"
    "ldp   x19, x20, [sp], #16\n\t"
    // sp now points to where we were before prologue
    "br    x10\n\t"                       // jump to saved PC

    // pcWasSet: entering a function (terminateMe) from its START.
    // suspendedSP may no longer be our save record: Process::kill pops
    // sendDesc-less frames with suspendedSP = lsf->sender(), leaving an
    // arbitrary frame*.  Both shapes have the frame link at [+0], so
    // restore only that, enter on an aligned sp at/below the record, and
    // leave the callee-saved registers alone (the function never returns
    // and the process is being torn down).
    "2:\n\t"
    "ldr   x29, [x9]\n\t"                // frame link from record/frame*
    "and   x9, x9, #0xfffffffffffffff0\n\t" // align down; record stays intact
    "mov   sp, x9\n\t"
    "mov   x30, #0\n\t"                  // no return; stop backtraces here
    "br    x10\n\t"                       // jump to entry point

    // --- Initialize new process ---
    "3:\n\t"
    "mov   x29, #0\n\t"                  // null frame link (stop frame tracing)
    "mov   sp, x9\n\t"                   // new stack pointer (0 mod 16)
#ifdef __APPLE__
    "adrp  x9, _ReturnOffTopOfProcess@PAGE\n\t"
    "add   x9, x9, _ReturnOffTopOfProcess@PAGEOFF\n\t"
#else
    "adrp  x9, ReturnOffTopOfProcess\n\t"
    "add   x9, x9, :lo12:ReturnOffTopOfProcess\n\t"
#endif
    "mov   x30, x9\n\t"                  // lr = ReturnOffTopOfProcess
    "br    x10\n\t"                       // jump to entry point

    ::: "memory"
  );
}


// =====================================================================
// JIT stubs -- fatal without JIT compiler
// =====================================================================

# ifdef SIC_COMPILER
// implemented in stubs_aarch64.cpp
extern oop (*EnterSelf_generated)(oop recv, char* entryPoint, oop arg1);
extern oop (*EnterSelfN_generated)(oop recv, char* entryPoint, oop* args, int32 nargs);
extern const fint EnterSelfMaxArgs;
extern void generate_EnterSelf();
# endif

extern "C" oop SendMessage_stub(...) {
  fatal("SendMessage_stub called without JIT");
  return NULL;
}

extern "C" oop SendDIMessage_stub(...) {
  fatal("SendDIMessage_stub called without JIT");
  return NULL;
}

extern "C" oop Recompile_stub(...) {
  fatal("Recompile_stub called without JIT");
  return NULL;
}

extern "C" oop DIRecompile_stub(...) {
  fatal("DIRecompile_stub called without JIT");
  return NULL;
}

extern "C" oop MakeOld_stub(...) {
  fatal("MakeOld_stub called without JIT");
  return NULL;
}

// ---------------------------------------------------------------------------
// Return-trap glue (return-into-a-patched-compiled-frame).
//
// frame::patch_compiled_self_frame overwrites a compiled Self frame F's
// saved return address with ReturnTrap and stashes F's real return PC in the
// caller's reserved currentPC slot.  When F's epilogue runs
// (restoreFrameAndReturn: `add sp,fp,8; ldr fp,[sp,-8]; ldr lr,[sp,0]; ret`)
// it returns here with:
//     x0 = result,  sp = F.fp + 8,  x29 = F's caller's fp.
// So the patched frame F is sp-8.  We build a small VM frame whose sender()
// is F (so Stack::last_self_frame finds F), then call the shared C++
// HandleReturnTrap, which never returns -- it resumes via
// ContinueAfterReturnTrap / ContinueNLRAfterReturnTrap.

extern bool8 processSemaphore;   // process.hh

extern "C" __attribute__((naked)) void ReturnTrap() {
  __asm__ __volatile__(
    // A non-local return through a patched frame diverts to savedPC +
    // sendDesc::non_local_return_offset (+8): the frame's NLR epilogue does
    // `ldr lr,[sp]; add lr,lr,#8; ret`.  So the stub mirrors a send site's
    // shape: normal entry at +0, NLR entry at +8.
    "b     3f\n\t"                 // +0: normal-return entry
    "nop\n\t"                      // +4: (mask-word slot in real sendDescs)
    // +8: NLR entry.  The NLR epilogue's geometry equals a normal return's:
    // sp = F.fp + 8, so F = sp - 8.  NLR regs live: x0 = result,
    // x1 = NLRHomeReg, x2 = NLRHomeIDReg.
    // F is taken from the UNROUNDED entry sp; our own sp is then rounded
    // down to 16 so the stores below satisfy Apple's hardware sp-alignment
    // check even when the patched record sits at 0 mod 16 (a kill patch
    // applied through the C-boundary alias record; ContinueNLRFromC lands
    // sp = record + 8, which is then 8 mod 16).
    "sub   x10, sp, #8\n\t"        // x10 = F (patched frame fp)
    "sub   x11, sp, #32\n\t"       // frame record (see normal entry comment)
    "and   x11, x11, #0xfffffffffffffff0\n\t"
    "mov   sp, x11\n\t"
    "str   x10, [sp, #0]\n\t"      // [fp+0] = F  -> our sender() is F
    "adr   x11, 1f\n\t"
    "str   x11, [sp, #8]\n\t"      // [fp+8] = marker (not a Self frame)
    "mov   x29, sp\n\t"
    "mov   x3, x1\n\t"             // arg3: nlrHome   (before x1 is clobbered)
    "mov   x4, x2\n\t"             // arg4: nlrHomeID (before x2 is clobbered)
    "mov   x1, x10\n\t"            // arg1: sp_of_patched_frame = F
    "mov   x2, #1\n\t"             // arg2: nlr = true
    "bl    " C_SYM("HandleReturnTrap") "\n\t"  // arg0 (x0) already = result
    "1:\n\t"
    "brk   #0x4e\n\t"              // HandleReturnTrap must not return
    "3:\n\t"                       // ---- normal-return entry ----
    "sub   x10, sp, #8\n\t"        // x10 = F (patched frame fp)
    // Reserve 32 (not 16) bytes: F's preserved words live at [F.fp+0] (saved
    // fp, read by the stack walk) and [F.fp+8] (the patched lr) -- i.e. at
    // sp-8 and sp on entry.  A 16-byte frame here would put [fp+8] right on
    // [F.fp+0] and clobber F's saved fp.  32 bytes keeps our record below
    // those words; the space is F's just-freed locals, safe to reuse.
    "sub   sp, sp, #32\n\t"        // our frame record (stays 16-aligned)
    "str   x10, [sp, #0]\n\t"      // [fp+0] = F  -> our sender() is F
    "adr   x11, 2f\n\t"            // a non-code-zone marker pc
    "str   x11, [sp, #8]\n\t"      // [fp+8] = marker (so we are not a Self frame)
    "mov   x29, sp\n\t"            // x29 = our frame
    "mov   x1, x10\n\t"           // arg1: sp_of_patched_frame = F
    "mov   x2, #0\n\t"            // arg2: nlr = false
    "mov   x3, #0\n\t"            // arg3: nlrHome = NULL
    "mov   x4, #0\n\t"            // arg4: nlrHomeID = 0
    "bl    " C_SYM("HandleReturnTrap") "\n\t" // arg0 (x0) already = result
    "2:\n\t"
    "brk   #0x4e\n\t"             // HandleReturnTrap must not return
  );
}

// Resume normal execution after a return trap: restore the caller's frame
// pointer and stack, and branch to the continuation PC with the result in
// x0.  Mirrors i386 ContinueAfterReturnTrap.  sp_arg is the patched frame F.
extern "C" __attribute__((naked, noreturn))
void ReturnTrap_resume(oop result, char* pc, char* sp_arg) {
  // naked: args are referenced directly by register (x0=result, x1=pc, x2=sp)
  // sp_arg (self_sp) is sp_of_patched_frame as formed by the trap stub:
  //   ReturnTrap        (returned frame):  self_sp = F.fp        = entry_sp - 8
  //   PrimCallReturnTrap(active   frame):  self_sp = entry_sp - 16
  // where entry_sp is the frame's 16-aligned running sp.  In both cases the
  // correct resume sp is entry_sp, i.e. the next 16-aligned boundary above
  // self_sp: (self_sp & ~15) + 16.  This is identical to the old "self_sp + 8"
  // when self_sp is 8 mod 16 (ReturnTrap), and fixes the active-frame
  // single-step case (self_sp 0 mod 16), which "+8" left 8-misaligned and so
  // tripped Apple arm64's hardware SP-alignment check on the next sp store.
  __asm__ __volatile__(
    "ldr   x29, [x2]\n\t"        // fp = [self_sp]  (uses original, unrounded x2)
    "and   x2, x2, #0xfffffffffffffff0\n\t" // round down to 16
    "add   x2, x2, #16\n\t"      // + 16 = trap entry sp (the frame's running sp)
    "mov   sp, x2\n\t"           // 16-aligned for both returned and active frames
    "br    x1\n\t"               // continuation PC; x0 still holds the result
  );
}

extern "C" void volatile ContinueAfterReturnTrap(oop result, char* pc, char* sp) {
  processSemaphore = false;
  ReturnTrap_resume(result, pc, sp);
}

extern "C" void ReturnTrap2() {
  fatal("ReturnTrap2 called without JIT");
}

extern "C" __attribute__((naked)) void PrimCallReturnTrap() {
  // Single-step debugger trap: the patched frame F is still active (a callee
  // returned into its send site).  The entry sp is running_sp + 8 when a C
  // prim returns (the true prim-call geometry) but running_sp when the
  // returner uses the method-epilogue shape -- the patch-time sendee
  // classification does not always match the actual returner, and no parity
  // rule works either (JIT records are 8 mod 16 but EnterSelf boundary
  // records are 0 mod 16).  So pass sp-16 and let get_patched_self_frame
  // disambiguate {sp-16, sp-8} by content: the true record's saved-pc slot
  // still holds the patched trap address (is_patched()).  A fixed sp-16
  // delivered F = frame-8 for the epilogue shape, which poisoned
  // is_patched()/currentPC reads, the conversion's sp, the watermark walk,
  // and the resume's fp load (the resumed frame ran on a stale x29 and all
  // later walks disagreed with the canonical vframeOops by 8). -- rca
  // Shaped like ReturnTrap: normal entry +0, NLR entry +8.
  //
  // Finding F: the returner loaded its lr from the patched slot [F + 8],
  // whose memory still holds this trap's entry address at this instant (the
  // is_patched() invariant).  WHERE the returner's epilogue leaves sp
  // relative to F is pure C-compiler geometry: clang keeps the frame record
  // at the frame TOP (sp lands at F + 16, or F + 8 for the method-epilogue
  // shape), but gcc builds the record at the frame BOTTOM and its epilogue
  // (ldp x29,x30,[sp],#framesize) leaves sp an entire dead frame above F.
  // So SCAN down from sp for the patched slot -- here in the stub, while
  // the dead frame is still intact -- and then place our own frame BELOW
  // the found record: the old fixed sub-32 reservation only protected the
  // clang geometries, and once C code ran, its frames overwrote the true
  // record, destroying the evidence every later walk needed.  Probe order
  // keeps the old precedence: F = sp-16, then sp-8, then downward.
  __asm__ __volatile__(
    "0:\n\t"
    "b     3f\n\t"                 // +0: normal (prim-call-return) entry
    "nop\n\t"                      // +4
    // +8: NLR entry -- x0 = result, x1 = NLRHomeReg, x2 = NLRHomeIDReg.
    "adr   x13, 0b\n\t"            // this trap's entry (the patched value)
    "bl    6f\n\t"                 // -> x10 = F (clobbers x11, x12, x14)
    "adr   x11, 1f\n\t"
    "str   x11, [sp, #8]\n\t"      // [fp+8] = marker (not a Self frame)
    "mov   x3, x1\n\t"             // arg3: nlrHome
    "mov   x4, x2\n\t"             // arg4: nlrHomeID
    "mov   x1, x10\n\t"            // arg1: sp_of_patched_frame = F
    "mov   x2, #1\n\t"             // arg2: nlr = true
    "bl    " C_SYM("HandleReturnTrap") "\n\t"
    "1:\n\t"
    "brk   #0x4f\n\t"
    "3:\n\t"                       // ---- normal entry ----
    "adr   x13, 0b\n\t"            // this trap's entry (the patched value)
    "bl    6f\n\t"                 // -> x10 = F (clobbers x11, x12, x14)
    "adr   x11, 2f\n\t"
    "str   x11, [sp, #8]\n\t"      // [fp+8] = marker (not a Self frame)
    "mov   x1, x10\n\t"            // arg1: sp_of_patched_frame = F
    "mov   x2, #0\n\t"             // arg2: nlr = false
    "mov   x3, #0\n\t"
    "mov   x4, #0\n\t"
    "bl    " C_SYM("HandleReturnTrap") "\n\t"
    "2:\n\t"
    "brk   #0x4f\n\t"
    // ---- shared: find F and build our frame record below it ----
    // In:  x13 = trap entry address; x30 = caller (not preserved -- both
    //      callers never return).  Out: x10 = F; sp/x29 = our record, with
    //      [sp+0] = F linked for the stack walks ([sp+8] filled by caller).
    "6:\n\t"
    "ldur  x11, [sp, #-8]\n\t"     // [ (sp-16) + 8 ]: preferred geometry
    "sub   x10, sp, #16\n\t"
    "cmp   x11, x13\n\t"
    "b.eq  7f\n\t"
    "ldr   x11, [sp]\n\t"          // [ (sp-8) + 8 ]: method-epilogue shape
    "sub   x10, sp, #8\n\t"
    "cmp   x11, x13\n\t"
    "b.eq  7f\n\t"
    "sub   x11, sp, #16\n\t"       // gcc shapes: scan slots downward
    "mov   x12, #128\n\t"          //   bounded by 128 words (1 KB)
    "8:\n\t"
    "ldr   x14, [x11]\n\t"
    "cmp   x14, x13\n\t"
    "b.eq  9f\n\t"
    "sub   x11, x11, #8\n\t"
    "subs  x12, x12, #1\n\t"
    "b.ne  8b\n\t"
    "brk   #0x50\n\t"              // no patched record: invariant broken
    "9:\n\t"
    // Found a gcc-shape record a whole dead frame below sp.  Relocate it to
    // the clang-canonical spot (sp-16): every downstream consumer -- the
    // resume's sp = F + 16 (must equal THIS entry sp: the Self caller's
    // running sp), the walks, the unpatch -- was built for that geometry.
    // The caller-fp-relative patch stashes (currentPC etc.) ride along via
    // the copied saved-fp word.  The source is at least sp-24, so it cannot
    // overlap the destination.
    "ldp   x12, x14, [x11, #-8]\n\t" // {saved-fp, patched-lr} of the record
    "sub   x10, sp, #16\n\t"       // F = sp - 16 (canonical)
    "stp   x12, x14, [x10]\n\t"
    "7:\n\t"
    "sub   x11, x10, #16\n\t"      // our record goes BELOW F ...
    "and   x11, x11, #0xfffffffffffffff0\n\t" // ... 16-aligned
    "mov   sp, x11\n\t"
    "str   x10, [sp, #0]\n\t"      // [fp+0] = F -> our sender() is F
    "mov   x29, sp\n\t"
    "ret\n\t"
  );
}

extern "C" void ProfilerTrap() {
  fatal("ProfilerTrap called without JIT");
}

// Continue a non-local return after a return trap: restore the caller's frame
// pointer and stack from the patched frame F (sp_arg), load the NLR register
// triple, and branch to pc -- the send site's NLR entry (sendDesc +
// non_local_return_offset), exactly where F's unpatched NLR epilogue would
// have gone.  Same sp rounding as ReturnTrap_resume: the next 16-aligned
// boundary above F is the frame's running sp for either trap convention.
//  -- rca 6/26
extern "C" __attribute__((naked, noreturn))
void ReturnTrapNLR_resume(char* pc, char* sp_arg, oop result,
                          frame* home, smi homeID) {
  // naked: x0=pc, x1=sp_arg(F), x2=result, x3=home, x4=homeID
  __asm__ __volatile__(
    "ldr   x29, [x1]\n\t"          // caller fp = [F] (original, unrounded x1)
    "and   x16, x1, #0xfffffffffffffff0\n\t"
    "add   x16, x16, #16\n\t"      // trap entry sp (16-aligned)
    "mov   x9, x0\n\t"             // continuation pc to scratch
    "mov   x0, x2\n\t"             // NLRResultReg
    "mov   x1, x3\n\t"             // NLRHomeReg
    "mov   x2, x4\n\t"             // NLRHomeIDReg
    "mov   sp, x16\n\t"
    "br    x9\n\t"
  );
}

extern "C" void volatile ContinueNLRAfterReturnTrap(char* pc, char* sp, oop result,
                                                     frame* home, int32 homeID) {
  processSemaphore = false;
  ReturnTrapNLR_resume(pc, sp, result, home, homeID);
}

// set by generate_EnterSelf() (stubs_aarch64.cpp) when the stub is
// emitted into the zone
extern "C" { char* firstSelfFrame_returnPC     = NULL; }
extern "C" { char* firstSelfFrameSendDescEnd   = NULL; }

// CallPrimitiveFromInterpreter: marshal args from interpreter stack to
// the C calling convention and call the primitive function.
//   entry_point = primitive function pointer
//   rcv         = receiver oop
//   argp        = pointer to args on interpreter stack (argp[0] = first arg)
//   nargs       = number of non-receiver arguments
//
// IMPORTANT: On ARM64, variadic function pointers (...) use a different
// calling convention (args on stack) than non-variadic (args in registers).
// We must cast to properly-typed non-variadic function pointers so the
// compiler generates correct register-based argument passing.
extern "C" oop CallPrimitiveFromInterpreter(void* entry_point, oop rcv,
                                             oop* argp, fint nargs) {
  typedef oop (*fn0)(oop);
  typedef oop (*fn1)(oop, oop);
  typedef oop (*fn2)(oop, oop, oop);
  typedef oop (*fn3)(oop, oop, oop, oop);
  typedef oop (*fn4)(oop, oop, oop, oop, oop);
  typedef oop (*fn5)(oop, oop, oop, oop, oop, oop);
  typedef oop (*fn6)(oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn7)(oop, oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn8)(oop, oop, oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn9)(oop, oop, oop, oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn10)(oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn11)(oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn12)(oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn13)(oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn14)(oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop);
  typedef oop (*fn15)(oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop, oop);
  switch (nargs) {
    case  0: return ((fn0)entry_point)(rcv);
    case  1: return ((fn1)entry_point)(rcv, argp[0]);
    case  2: return ((fn2)entry_point)(rcv, argp[0], argp[1]);
    case  3: return ((fn3)entry_point)(rcv, argp[0], argp[1], argp[2]);
    case  4: return ((fn4)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3]);
    case  5: return ((fn5)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4]);
    case  6: return ((fn6)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5]);
    case  7: return ((fn7)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6]);
    case  8: return ((fn8)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7]);
    case  9: return ((fn9)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8]);
    case 10: return ((fn10)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8], argp[9]);
    case 11: return ((fn11)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8], argp[9], argp[10]);
    case 12: return ((fn12)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8], argp[9], argp[10], argp[11]);
    case 13: return ((fn13)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8], argp[9], argp[10], argp[11], argp[12]);
    case 14: return ((fn14)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8], argp[9], argp[10], argp[11], argp[12], argp[13]);
    case 15: return ((fn15)entry_point)(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8], argp[9], argp[10], argp[11], argp[12], argp[13], argp[14]);
    default:
      fatal("CallPrimitiveFromInterpreter: too many arguments");
      return NULL;
  }
}

extern "C" oop EnterSelf(oop recv, char* entryPoint, oop arg1) {
# ifdef SIC_COMPILER
  if (EnterSelf_generated == NULL) generate_EnterSelf();
  // the thread's default W^X state is execute; every zone writer brackets
  // itself with a counted JITWriteScope, so no toggle is needed here
  return EnterSelf_generated(recv, entryPoint, arg1);
# else
  Unused(recv); Unused(entryPoint); Unused(arg1);
  fatal("EnterSelf called without JIT");
  return NULL;
# endif
}

// Multi-argument C -> compiled entry: marshals nargs args from the C array
// into a fresh Self outgoing area.  Shares EnterSelf's return point + epilogue.
extern "C" oop EnterSelfN(oop recv, char* entryPoint, oop* args, int32 nargs) {
# ifdef SIC_COMPILER
  if (EnterSelfN_generated == NULL) generate_EnterSelf();
  assert(nargs <= EnterSelfMaxArgs, "EnterSelfN arg count exceeds outgoing area");
  return EnterSelfN_generated(recv, entryPoint, args, nargs);
# else
  Unused(recv); Unused(entryPoint); Unused(args); Unused(nargs);
  fatal("EnterSelfN called without JIT");
  return NULL;
# endif
}

// Unwind the C/VM frames and resume an NLR in compiled Self code.
// addr is the send-site return PC that was live when Self called into the
// VM, i.e. the saved-lr slot of the VM's entry record.  We walk our own
// frame chain to that record -- SwitchStack leaves x29 alone, so the chain
// spans the VM/process stack switch -- then restore the Self frame's fp/sp
// and branch to the send site's NLR entry (return PC + 8; see
// non_local_return_offset in sendDesc_aarch64.hh).
//
// Args land exactly in the NLR register convention (regs_aarch64.hh):
// x0 = NLRResultReg, x1 = NLRHomeReg, x2 = NLRHomeIDReg.

extern "C" {
  extern oop   NLRResultFromC;   // set by NLRSupport::save_NLR_results
  extern smi   NLRHomeFromC;
  extern int32 NLRHomeIDFromC;
}
extern bool8 processSemaphore;   // process.hh

// Final hop: install the computed fp/sp and branch to the NLR target.
// x0..x2 already hold the NLR register convention values.
extern "C" __attribute__((naked, noreturn))
void ContinueNLR_jump(oop result, smi home, int32 homeID,
                      char* target, frame* new_fp, char* new_sp) {
  // naked: args are referenced directly by register (x0..x5)
  __asm__ __volatile__(
    "mov   x29, x4\n\t"
    "mov   sp,  x5\n\t"
    "br    x3\n\t"
  );
}

static void ContinueNLR_unwind_and_jump(oop result, smi home, int32 homeID,
                                        char* match, char* target) {
  Unused(result); Unused(home); Unused(homeID);
  // Walk our own frame chain to the record whose saved lr == match.
  // SwitchStack leaves x29 alone, so the chain spans the VM/process
  // stack switch.
  char** x9 = (char**)__builtin_frame_address(0);
  while (x9 != NULL  &&  x9[1] != match)
    x9 = (char**)x9[0];
  if (x9 == NULL)
    fatal1("ContinueNLR: return PC %#lx not on the frame chain", match);

  // Pop the matched record.  How much to pop depends on who built it:
  // a JIT prologue record straddles the boundary -- its saved-lr word at
  // [x9+8] is the caller's reserved lr-hole, so the sp just above it is
  // x9+8, the caller's running sp.  An EnterSelf/C boundary record is a
  // self-contained pair, so the sp above it is x9+16.  Disambiguate by
  // content first: a patched return address only ever lives in a Self
  // frame record (frame::patch_compiled_self_frame), even one a
  // conversion rebuilt at 0 mod 16.  For unpatched matches fall back to
  // parity, "the next 16-aligned boundary at or above x9+8" (same
  // rounding as ReturnTrap_resume): JIT records are 8 mod 16, boundary
  // records 0 mod 16.  A flat x9+16 left the JIT case 8-misaligned and
  // tripped Apple's hardware sp-alignment check at the target's first
  // sp-relative store (e.g. ReturnTrap+0x10).
  char* new_sp = isPatchedReturnAddress(match)
      ?  (char*)x9 + oopSize
      :  (char*)(((uintptr_t)x9 + 16) & ~(uintptr_t)15);
  ContinueNLR_jump(NLRResultFromC, NLRHomeFromC, NLRHomeIDFromC,
                   target, (frame*)x9[0], new_sp);
  ShouldNotReachHere();
}

extern "C" oop volatile ContinueNLRFromC(char* addr, bool isInterpreted, bool isSelfIC) {
  Unused(isSelfIC);  // send and primitive descs both put NLR code at +8
  if (isInterpreted)
    fatal("interpreted NLR uses longjmp on 64-bit "
          "(see continue_NLR_into_interpreted_Self)");
  processSemaphore = false;
  ContinueNLR_unwind_and_jump(NLRResultFromC, NLRHomeFromC, NLRHomeIDFromC,
                              addr, addr + 8 /*non_local_return_offset*/);
  return NULL; // not reached
}

// DiscardStack, check_saved_byte_map_base, set_flags_for_platform,
// and DIRecompile_stub_returnPC are already defined in runtime_i386.cpp
// (which compiles for aarch64 too).
// continuePC is defined in runtime.cpp.

// JIT global variables only defined in the assembly .S files (now excluded):
char* ReturnTrap_returnPC       = NULL;
char* ReturnTrapNLR_returnPC    = NULL;
char* Recompile_stub_returnPC   = NULL;
char* MakeOld_stub_returnPC     = NULL;
char* SendMessage_stub_returnPC = NULL;

# if !defined(FAST_COMPILER) && !defined(SIC_COMPILER)
// zone::frame_chain_nesting static member (defined in zone.cpp when the
// real zone is compiled in)
int32 zone::frame_chain_nesting = 0;
# endif

# endif // TARGET_ARCH == AARCH64_ARCH
