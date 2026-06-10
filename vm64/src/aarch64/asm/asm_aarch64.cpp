# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "asm_aarch64.hh"

# include "_asm_aarch64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

Assembler* theAssembler;      // current assembler for instructions

Assembler::Assembler(int32 instsSize, int32 locsSize, bool pr, bool isInstrs)
  : BaseAssembler(instsSize, locsSize, pr, isInstrs) {}


void Assembler::Backpatch(pc_t destp, pc_t target) {
  Unused(destp); Unused(target);
  fatal("aarch64 Assembler::Backpatch not yet implemented");
}


// Type-test counting instrumentation: not implemented on aarch64
// (i386 didn't implement it either; only used under SICCountTypeTests).
void Assembler::startTypeTest(fint ncases, bool prologueCheck, bool immedOnly) {
  Unused(ncases); Unused(prologueCheck); Unused(immedOnly);
}
void Assembler::doOneTypeTest() {}
void Assembler::endTypeTest() {}


// print_code: annotated code disassembly, not yet implemented
void print_code(nmethod* nm, pc_t start, pc_t end) {
  Unused(nm); Unused(start); Unused(end);
  lprintf("[print_code: aarch64 disassembly not yet implemented]\n");
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == AARCH64_ARCH
