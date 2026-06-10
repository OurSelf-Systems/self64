# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_sendDesc_aarch64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

void sendDesc::init_platform() {
  // jump_address words must be 8-aligned (see sendDesc_aarch64.hh)
  assert((jump_address_offset & (oopSize - 1)) == 0, "sendDesc alignment incorrect");
}


// On aarch64 the call target is a plain address word in the sendDesc data
// block, loaded by the ldr/blr call sequence (see sendDesc_aarch64.hh).
// Rebinding is an atomic 8-byte store; no instruction decoding, no icache
// flush.

char* sendDesc::jump_addr() {
  return *jump_addr_addr();
}

void sendDesc::set_jump_addr(char* t) {
  *jump_addr_addr() = t;
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == AARCH64_ARCH
