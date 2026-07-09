# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "diDesc_amd64.hh"
# include "_diDesc_amd64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

// On amd64 the DI jump target is a plain address word reached by the
// JMP [rip+0] call sequence (see diDesc_amd64.hh), so these are simple
// loads/stores; no instruction decoding and no icache flush on repatch.

pc_t DIDesc::jump_addr() {
  return *(pc_t*)jump_addr_addr();
}


void DIDesc::set_jump_addr(pc_t insts) {
  JITWriteScope ws;
  *(pc_t*)jump_addr_addr() = insts;
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == X86_64_ARCH
