# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "diDesc_aarch64.hh"
# include "_diDesc_aarch64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

// On aarch64 the DI jump target is a plain address word loaded by the
// call sequence (see diDesc_aarch64.hh), so these are simple loads/stores;
// no instruction decoding and no icache flush on repatch.

pc_t DIDesc::jump_addr() {
  return *(pc_t*)jump_addr_addr();
}


void DIDesc::set_jump_addr(pc_t insts) {
  JITWriteScope ws;
  *(pc_t*)jump_addr_addr() = insts;
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == AARCH64_ARCH
