/* Copyright 2024-2026 AUTHORS.
   See the LICENSE file for license information. */

/* C/asm stubs for x86_64 interpreter-only builds.
   Replaces i386/runtime/runtime_asm_gcc_i386.S */

# include "config.hh"
# if TARGET_ARCH == X86_64_ARCH

# include "_runtime_amd64.cpp.incl"


// =====================================================================
// Frame pointer access
// =====================================================================

extern "C" frame* currentFrame() {
  // Return the caller's frame pointer.
  // The i386 asm version just returns %ebp (caller's, since it's a leaf).
  // We dereference rbp once to get past our own frame setup.
  frame* fp;
  __asm__ __volatile__("movq (%%rbp), %0" : "=r"(fp));
  return fp;
}

extern "C" char* currentReturnAddr() {
  return (char*)__builtin_return_address(0);
}


// =====================================================================
// Stack switching (SwitchStack)
// =====================================================================
// These switch to a new stack (the VM stack) and call a function there.
// They must be implemented in assembly because they change %rsp.

// SwitchStack0: call fn_start on newSP, no extra args
extern "C" char* SwitchStack0(char* fn_start, char* newSP) {
  char* result;
  __asm__ __volatile__(
    "movq   %%rsp, %%r10\n\t"     // save old SP
    "movq   %%rbp, %%r11\n\t"     // save old BP
    "movq   %2,    %%rsp\n\t"     // switch to new stack
    "pushq  %%r10\n\t"            // save old SP on new stack
    "pushq  %%r11\n\t"            // save old BP on new stack
    "callq  *%1\n\t"              // call fn_start()
    "popq   %%r11\n\t"            // restore old BP
    "popq   %%rsp\n\t"            // restore old SP
    "movq   %%rbp, %%r11\n\t"     // (rbp may have changed)
    : "=a"(result)                 // result in rax
    : "r"(fn_start), "r"(newSP)
    : "r10", "r11", "rcx", "rdx", "rsi", "rdi", "r8", "r9",
      "memory", "cc"
  );
  return result;
}

extern "C" char* SwitchStack1(char* fn_start, char* newSP, void* arg1) {
  char* result;
  __asm__ __volatile__(
    "movq   %%rsp, %%r10\n\t"
    "movq   %2,    %%rsp\n\t"
    "pushq  %%r10\n\t"
    "subq   $8,    %%rsp\n\t"     // 16-byte align for SysV ABI
    "movq   %3,    %%rdi\n\t"     // arg1 in rdi (SysV ABI)
    "callq  *%1\n\t"
    "addq   $8,    %%rsp\n\t"     // undo alignment pad
    "popq   %%rsp\n\t"
    : "=a"(result)
    : "r"(fn_start), "r"(newSP), "r"(arg1)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "memory", "cc"
  );
  return result;
}

extern "C" char* SwitchStack2(char* fn_start, char* newSP, void* arg1, void* arg2) {
  char* result;
  __asm__ __volatile__(
    "movq   %%rsp, %%r10\n\t"
    "movq   %2,    %%rsp\n\t"
    "pushq  %%r10\n\t"
    "subq   $8,    %%rsp\n\t"     // 16-byte align for SysV ABI
    "movq   %3,    %%rdi\n\t"     // arg1 in rdi
    "movq   %4,    %%rsi\n\t"     // arg2 in rsi
    "callq  *%1\n\t"
    "addq   $8,    %%rsp\n\t"     // undo alignment pad
    "popq   %%rsp\n\t"
    : "=a"(result)
    : "r"(fn_start), "r"(newSP), "r"(arg1), "r"(arg2)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "memory", "cc"
  );
  return result;
}

extern "C" char* SwitchStack3(char* fn_start, char* newSP,
                               void* arg1, void* arg2, void* arg3) {
  char* result;
  __asm__ __volatile__(
    "movq   %%rsp, %%r10\n\t"
    "movq   %2,    %%rsp\n\t"
    "pushq  %%r10\n\t"
    "subq   $8,    %%rsp\n\t"     // 16-byte align for SysV ABI
    "movq   %3,    %%rdi\n\t"
    "movq   %4,    %%rsi\n\t"
    "movq   %5,    %%rdx\n\t"     // arg3 in rdx
    "callq  *%1\n\t"
    "addq   $8,    %%rsp\n\t"     // undo alignment pad
    "popq   %%rsp\n\t"
    : "=a"(result)
    : "r"(fn_start), "r"(newSP), "r"(arg1), "r"(arg2), "r"(arg3)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "memory", "cc"
  );
  return result;
}

extern "C" char* SwitchStack4(char* fn_start, char* newSP,
                               void* arg1, void* arg2,
                               void* arg3, void* arg4) {
  char* result;
  __asm__ __volatile__(
    "movq   %%rsp, %%r10\n\t"
    "movq   %2,    %%rsp\n\t"
    "pushq  %%r10\n\t"
    "subq   $8,    %%rsp\n\t"     // 16-byte align for SysV ABI
    "movq   %3,    %%rdi\n\t"
    "movq   %4,    %%rsi\n\t"
    "movq   %5,    %%rdx\n\t"
    "movq   %6,    %%rcx\n\t"     // arg4 in rcx
    "callq  *%1\n\t"
    "addq   $8,    %%rsp\n\t"     // undo alignment pad
    "popq   %%rsp\n\t"
    : "=a"(result)
    : "r"(fn_start), "r"(newSP), "r"(arg1), "r"(arg2),
      "r"(arg3), "r"(arg4)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "memory", "cc"
  );
  return result;
}


// =====================================================================
// Process switching (SetSPAndCall)
// =====================================================================
// This is the heart of cooperative multitasking in Self.
// Saves/restores ALL callee-saved registers for process switching.
//
// Args: rdi=callerSaveAddr, rsi=calleeSaveAddr, edx=init,
//       rcx=semaphore, r8b=pcWasSet
//
// callerSaveAddr[0]/[1] = saved rsp / saved return addr
// calleeSaveAddr[0]/[1] = saved rsp / saved return addr (or new sp / entry)
//
// The saved rsp (= suspendedSP) must be usable as a frame* by frame-walking
// code (Stack::first_VM_frame, etc.), so it must be:
//   - 16-byte aligned (frame::is_aligned())
//   - at *(suspendedSP+0): saved rbp (used by sender())
//   - at *(suspendedSP+8): return addr = 0 (marks it as non-Self frame)
//
// Saved stack layout (at callerSaveAddr[0]):
//   [original return addr]   <- rbp+8
//   [saved rbp]              <- rbp
//   [saved rbx]              <- rbp-8
//   [saved r12]              <- rbp-16
//   [saved r13]              <- rbp-24
//   [saved r14]              <- rbp-32
//   [saved r15]              <- rbp-40
//   [padding = 0]            <- rbp-48
//   [mock return addr = 0]   <- rbp-56
//   [copy of rbp]            <- rbp-64 = callerSaveAddr[0] (0 mod 16)

extern "C" void ReturnOffTopOfProcess();

extern "C" __attribute__((naked))
void SetSPAndCall(char** callerSaveAddr, char** calleeSaveAddr,
                  bool init, bool8* semaphore, bool8 pcWasSet) {
  __asm__ __volatile__(
    // Prologue: save all callee-saved registers
    "pushq %%rbp\n\t"
    "movq  %%rsp, %%rbp\n\t"
    "pushq %%rbx\n\t"
    "pushq %%r12\n\t"
    "pushq %%r13\n\t"
    "pushq %%r14\n\t"
    "pushq %%r15\n\t"

    // Create frame-like footer for external frame walkers
    // (so suspendedSP can be interpreted as a frame* by Stack::first_VM_frame)
    "pushq $0\n\t"                    // padding (alignment)
    "pushq $0\n\t"                    // mock return addr (marks non-Self frame)
    "pushq %%rbp\n\t"                 // rbp copy (sender() follows this)
    // rsp is now 0 mod 16

    // Save caller state if callerSaveAddr (rdi) != NULL
    "testq %%rdi, %%rdi\n\t"
    "jz    1f\n\t"
    "movq  %%rsp, (%%rdi)\n\t"        // callerSaveAddr[0] = rsp (frame-like)
    "movq  8(%%rbp), %%rax\n\t"       // caller's return address
    "movq  %%rax, 8(%%rdi)\n\t"       // callerSaveAddr[1] = return addr
    "1:\n\t"

    // Clear semaphore
    "movb  $0, (%%rcx)\n\t"

    // Load callee's saved state (read before switching stacks)
    "movq  (%%rsi), %%r9\n\t"         // saved SP (or new SP if init)
    "movq  8(%%rsi), %%rax\n\t"       // saved PC (or entry point if init)

    "testb %%dl, %%dl\n\t"            // init?
    "jnz   3f\n\t"

    // --- Resume existing process ---
    "testb %%r8b, %%r8b\n\t"          // pcWasSet?
    "jnz   2f\n\t"

    // Normal resume: suspendedSP is a genuine save record made by the
    // prologue above; pop it and jump to the saved PC.
    "movq  %%r9, %%rsp\n\t"           // switch to callee's saved stack
    "addq  $24, %%rsp\n\t"            // skip footer (rbp copy + mock ret + pad)
    "popq  %%r15\n\t"                 // restore all callee-saved registers
    "popq  %%r14\n\t"
    "popq  %%r13\n\t"
    "popq  %%r12\n\t"
    "popq  %%rbx\n\t"
    "popq  %%rbp\n\t"
    // rsp now points to original return address
    "movq  %%rax, (%%rsp)\n\t"        // overwrite with saved PC
    "ret\n\t"                          // pop return addr and jump

    // pcWasSet: entering a function (terminateMe) from its START.
    // suspendedSP may no longer be our save record: Process::kill pops
    // sendDesc-less frames with suspendedSP = lsf->sender(), leaving an
    // arbitrary frame*.  Both shapes have the frame link at [+0], so
    // restore only that, enter on a call-shaped rsp at/below the record,
    // and leave the callee-saved registers alone (the function never
    // returns and the process is being torn down).
    "2:\n\t"
    "movq  (%%r9), %%rbp\n\t"         // frame link from record/frame*
    "andq  $-16, %%r9\n\t"            // align down; record stays intact
    "movq  %%r9, %%rsp\n\t"
    "pushq $0\n\t"                    // fake return addr; rsp now 8 mod 16
    "jmpq  *%%rax\n\t"                // jump to entry point

    // --- Initialize new process ---
    // suspendedSP is 0 mod 16 on x86_64.  Push ReturnOffTopOfProcess (8 bytes)
    // so rsp becomes 8 mod 16, which is correct for a function entry (as if
    // call had just pushed a return address).
    "3:\n\t"
    "movq  $0, %%rbp\n\t"             // null frame link (stop frame tracing)
    "movq  %%r9, %%rsp\n\t"           // new stack pointer (0 mod 16)
    "leaq  ReturnOffTopOfProcess(%%rip), %%r9\n\t"
    "pushq %%r9\n\t"                   // return addr if process falls off top
    "jmpq  *%%rax\n\t"                // jump to entry point (rsp is 8 mod 16)

    ::: "memory"
  );
}


// =====================================================================
// JIT stubs — fatal without JIT compiler
// =====================================================================

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
// caller's reserved currentPC slot.  When F's epilogue runs (leave; ret) the
// RET consumes the patched slot itself, so on entry here
//     rax = result,  rsp = F + 16,  rbp = F's caller's fp
// -- ALWAYS, for every returner and every C compiler (the aarch64 stub's
// frame-record probing and gcc-geometry scanning have no x86 counterpart;
// cf. the i386 original in runtime_asm_gcc_i386.S, where ReturnTrap and
// PrimCallReturnTrap are the same label).  Re-pushing the trap address and
// rbp re-forms F's record in place (same values the memory still holds), so
// our record IS F and HandleReturnTrap's currentFrame()->sender() finds it.
//
// A non-local return through a patched frame diverts to the patched value +
// sendDesc::non_local_return_offset (+5), so the stub mirrors a send site's
// shape: normal entry at +0, NLR entry at +5 (the +0 jmp is emitted as an
// explicit rel32 so the NLR entry lands exactly at +5).

extern bool8 processSemaphore;   // process.hh

// The stub builds its own marker record BELOW F (as on aarch64), never
// re-forming F itself: HandleReturnTrap takes currentFrame()->the stub
// record and relinks its [+0] to F -- if the record WERE F, that relink
// writes [F+0] = F and the frame chain becomes a self-loop (this hung the
// first world build in Stack::first_VM_frame).  F's own words at [F+0] /
// [F+8] still hold the saved fp and the patched trap address (RET reads
// memory without erasing it), which the unpatcher and walks rely on.
// Defined in a top-level asm block, NOT as naked C functions: the compiler
// prepends endbr64 (-fcf-protection) to every C-level function, which shifts
// the entry layout -- the patched value is first_inst_addr(ReturnTrap) == the
// symbol itself, and an NLR diverts to symbol+5, which must be the second
// entry, not the middle of a rel32.  A top-level asm blob owns its first byte.
__asm__(
  "  .text\n"
  "  .align 8\n"
  "  .globl ReturnTrap\n"
  "  .type ReturnTrap, @function\n"
  "ReturnTrap:\n"
  "  .byte 0xe9\n"                  // +0: jmp .Lrt_normal -- forced rel32
  "  .long .Lrt_normal - . - 4\n"
  // == ReturnTrap + 5: the NLR entry.
  // NLR: rax = result, rdx = NLRHomeReg, rcx = NLRHomeIDReg
  "  subq  $32, %rsp\n"             // our record, below F's intact words
  "  leaq  16(%rsp), %r11\n"        // r11 = F (= entry rsp - 16)
  "  movq  %r11, (%rsp)\n"          // [fp+0] = F -> our sender() is F
  "  leaq  .Lrt_marker(%rip), %r10\n"
  "  movq  %r10, 8(%rsp)\n"         // [fp+8] = marker (not a Self frame)
  "  movq  %rsp, %rbp\n"            // rbp = our record (rsp = F-16, 0 mod 16)
  "  movq  %rcx, %r8\n"             // arg4: nlrHomeID (before rcx clobbered)
  "  movq  %rdx, %rcx\n"            // arg3: nlrHome   (before rdx clobbered)
  "  movl  $1, %edx\n"              // arg2: nlr = true
  "  movq  %r11, %rsi\n"            // arg1: sp_of_patched_frame = F
  "  movq  %rax, %rdi\n"            // arg0: result
  "  call  HandleReturnTrap\n"
  ".Lrt_marker:\n"
  "  hlt\n"                         // HandleReturnTrap must not return
  ".Lrt_normal:\n"                  // ---- normal-return entry ----
  "  subq  $32, %rsp\n"             // our record, below F's intact words
  "  leaq  16(%rsp), %r11\n"        // r11 = F
  "  movq  %r11, (%rsp)\n"          // [fp+0] = F -> our sender() is F
  "  leaq  .Lrt_marker2(%rip), %r10\n"
  "  movq  %r10, 8(%rsp)\n"         // [fp+8] = marker (not a Self frame)
  "  movq  %rsp, %rbp\n"            // rbp = our record
  "  movq  %rax, %rdi\n"            // arg0: result
  "  movq  %r11, %rsi\n"            // arg1: sp_of_patched_frame = F
  "  xorl  %edx, %edx\n"            // arg2: nlr = false
  "  xorl  %ecx, %ecx\n"            // arg3: nlrHome = NULL
  "  xorl  %r8d, %r8d\n"            // arg4: nlrHomeID = 0
  "  call  HandleReturnTrap\n"
  ".Lrt_marker2:\n"
  "  hlt\n"                         // HandleReturnTrap must not return
  "  .size ReturnTrap, . - ReturnTrap\n"

  // On x86 the RET consumed the patched slot whatever the returner was, so
  // the geometry is identical to ReturnTrap: alias both entries (the
  // patcher still needs a distinct symbol to choose per sendee kind).
  "  .align 8\n"
  "  .globl PrimCallReturnTrap\n"
  "  .type PrimCallReturnTrap, @function\n"
  "PrimCallReturnTrap:\n"
  "  .byte 0xe9\n"                  // +0 -> ReturnTrap+0 (normal entry)
  "  .long ReturnTrap - . - 4\n"
  "  .byte 0xe9\n"                  // +5 -> ReturnTrap+5 (NLR entry)
  "  .long ReturnTrap + 5 - . - 4\n"
  "  .size PrimCallReturnTrap, . - PrimCallReturnTrap\n"
);

extern "C" void ReturnTrap2() {
  fatal("ReturnTrap2 unused on x86_64");
}

extern "C" void ProfilerTrap() {
  fatal("ProfilerTrap called without JIT");
}

// Resume normal execution after a return trap: restore the caller's frame
// pointer, put sp back at the trap-entry position (F + 16, always), and
// jump to the continuation PC with the result in rax.
extern "C" __attribute__((naked, noreturn))
void ReturnTrap_resume(oop result, char* pc, char* sp_arg) {
  // naked: rdi = result, rsi = pc, rdx = sp_arg = F
  __asm__ __volatile__(
    "movq  (%rdx), %rbp\n\t"       // caller fp = [F]
    "leaq  16(%rdx), %rsp\n\t"     // resume sp = F + 16 (the trap entry sp)
    "movq  %rdi, %rax\n\t"         // result
    "jmpq  *%rsi\n\t"
  );
}

extern "C" void volatile ContinueAfterReturnTrap(oop result, char* pc, char* sp) {
  processSemaphore = false;
  ReturnTrap_resume(result, pc, sp);
}

// Continue a non-local return after a return trap: like ReturnTrap_resume
// but loading the NLR register triple; pc is the send site's NLR entry
// (sendDesc + non_local_return_offset), exactly where F's unpatched NLR
// epilogue would have gone.
extern "C" __attribute__((naked, noreturn))
void ReturnTrapNLR_resume(char* pc, char* sp_arg, oop result,
                          frame* home, smi homeID) {
  // naked: rdi = pc, rsi = F, rdx = result, rcx = home, r8 = homeID
  __asm__ __volatile__(
    "movq  (%rsi), %rbp\n\t"       // caller fp = [F]
    "leaq  16(%rsi), %rsp\n\t"     // resume sp = F + 16
    "movq  %rdx, %rax\n\t"         // NLRResultReg
    "movq  %rcx, %rdx\n\t"         // NLRHomeReg
    "movq  %r8,  %rcx\n\t"         // NLRHomeIDReg
    "jmpq  *%rdi\n\t"
  );
}

extern "C" void volatile ContinueNLRAfterReturnTrap(char* pc, char* sp, oop result,
                                                     frame* home, int32 homeID) {
  processSemaphore = false;
  ReturnTrapNLR_resume(pc, sp, result, home, homeID);
}

// data pointers on 64-bit (see runtime.hh); never set without a JIT
extern "C" { char* firstSelfFrame_returnPC     = NULL; }
extern "C" { char* firstSelfFrameSendDescEnd   = NULL; }

// CallPrimitiveFromInterpreter: marshal args from interpreter stack to
// the C calling convention and call the primitive function.
//   entry_point = primitive function pointer
//   rcv         = receiver oop
//   argp        = pointer to args on interpreter stack (argp[0] = first arg)
//   nargs       = number of non-receiver arguments
extern "C" oop CallPrimitiveFromInterpreter(void* entry_point, oop rcv,
                                             oop* argp, fint nargs) {
  typedef oop (*prim_fn_t)(...);
  prim_fn_t fn = (prim_fn_t)entry_point;
  switch (nargs) {
    case  0: return fn(rcv);
    case  1: return fn(rcv, argp[0]);
    case  2: return fn(rcv, argp[0], argp[1]);
    case  3: return fn(rcv, argp[0], argp[1], argp[2]);
    case  4: return fn(rcv, argp[0], argp[1], argp[2], argp[3]);
    case  5: return fn(rcv, argp[0], argp[1], argp[2], argp[3], argp[4]);
    case  6: return fn(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5]);
    case  7: return fn(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6]);
    case  8: return fn(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7]);
    case  9: return fn(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8]);
    case 10: return fn(rcv, argp[0], argp[1], argp[2], argp[3], argp[4], argp[5], argp[6], argp[7], argp[8], argp[9]);
    default:
      fatal("CallPrimitiveFromInterpreter: too many arguments");
      return NULL;
  }
}

# ifdef SIC_COMPILER
// implemented in stubs_amd64.cpp
extern oop (*EnterSelf_generated)(oop recv, char* entryPoint, oop arg1);
extern oop (*EnterSelfN_generated)(oop recv, char* entryPoint, oop* args, int32 nargs);
extern const fint EnterSelfMaxArgs;
extern void generate_EnterSelf();
# endif

extern "C" oop EnterSelf(oop recv, char* entryPoint, oop arg1) {
# ifdef SIC_COMPILER
  if (EnterSelf_generated == NULL) generate_EnterSelf();
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

extern "C" {
  extern oop   NLRResultFromC;   // set by NLRSupport::save_NLR_results
  extern smi   NLRHomeFromC;
  extern int32 NLRHomeIDFromC;
}

// Unwind the C/VM frames and resume an NLR in compiled Self code.
// addr is the send-site return PC that was live when Self called into the
// VM, i.e. the return-address slot of the VM's entry record.  We walk our
// own frame chain to that record -- SetSPAndCall keeps a walkable rbp
// chain, so it spans the VM/process stack switch -- then restore the Self
// frame's fp/sp and jump to the send site's NLR entry (return PC +
// non_local_return_offset; see sendDesc_amd64.hh).
//
// Unlike aarch64 there is no landing-sp ambiguity: every record on x86 is
// a pushed {rbp, retPC} pair, so the sp above the matched record is
// uniformly record + 16 -- for JIT records, boundary records, and
// gcc-compiled prims alike (this is the "one layer past
// PrimCallReturnTrap" gcc geometry problem not existing here).

// Final hop: install the computed fp/sp and jump to the NLR target with
// the NLR register convention (rax/rdx/rcx) loaded.
extern "C" __attribute__((naked, noreturn))
void ContinueNLR_jump(oop result, smi home, int32 homeID,
                      char* target, frame* new_fp, char* new_sp) {
  // naked: rdi=result, rsi=home, rdx=homeID, rcx=target, r8=new_fp, r9=new_sp
  __asm__ __volatile__(
    "movq  %r8, %rbp\n\t"
    "movq  %r9, %rsp\n\t"
    "movq  %rcx, %r10\n\t"         // target (rcx is about to be loaded)
    "movq  %rdi, %rax\n\t"         // NLRResultReg
    "movq  %rdx, %rcx\n\t"         // NLRHomeIDReg (read rdx before writing)
    "movq  %rsi, %rdx\n\t"         // NLRHomeReg
    "jmpq  *%r10\n\t"
  );
}

static void ContinueNLR_unwind_and_jump(oop result, smi home, int32 homeID,
                                        char* match, char* target) {
  Unused(result); Unused(home); Unused(homeID);
  // Walk our own frame chain to the record whose saved retPC == match.
  char** rec = (char**)__builtin_frame_address(0);
  while (rec != NULL  &&  rec[1] != match)
    rec = (char**)rec[0];
  if (rec == NULL)
    fatal1("ContinueNLR: return PC %#lx not on the frame chain", match);

  // Pop the matched {rbp, retPC} pair: the Self caller's running sp is
  // uniformly rec + 16 (see the comment above).
  char* new_sp = (char*)rec + 2 * oopSize;
  ContinueNLR_jump(NLRResultFromC, NLRHomeFromC, NLRHomeIDFromC,
                   target, (frame*)rec[0], new_sp);
  ShouldNotReachHere();
}

extern "C" oop volatile ContinueNLRFromC(char* addr, bool isInterpreted, bool isSelfIC) {
  Unused(isSelfIC);  // send and primitive descs both put NLR code at +5
  if (isInterpreted)
    fatal("interpreted NLR uses longjmp on 64-bit "
          "(see continue_NLR_into_interpreted_Self)");
  processSemaphore = false;
  ContinueNLR_unwind_and_jump(NLRResultFromC, NLRHomeFromC, NLRHomeIDFromC,
                              addr, addr + 5 /*non_local_return_offset*/);
  return NULL; // not reached
}

// DiscardStack, check_saved_byte_map_base, set_flags_for_platform,
// and DIRecompile_stub_returnPC are already defined in runtime_i386.cpp
// (which compiles for x86_64 too).
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

# endif // TARGET_ARCH == X86_64_ARCH
