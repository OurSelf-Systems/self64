# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_sendDesc_amd64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

void sendDesc::init_platform() {
  // jump_address words must be 8-aligned (see sendDesc_amd64.hh)
  assert((jump_address_offset & (oopSize - 1)) == 0, "sendDesc alignment incorrect");
}


// On amd64 the call target is a plain address word in the sendDesc data
// block, called through by CALL [rip+16] (see sendDesc_amd64.hh).
// Rebinding is an atomic 8-byte store; no instruction decoding, and the
// x86 instruction cache is coherent.

char* sendDesc::jump_addr() {
  return *jump_addr_addr();
}

void sendDesc::set_jump_addr(char* t) {
  JITWriteScope ws;
  *jump_addr_addr() = t;
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == X86_64_ARCH
