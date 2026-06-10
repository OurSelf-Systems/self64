# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "fields_aarch64.hh"

# include "_fields_aarch64.cpp.incl"

char* address_of_overwritten_NIC_save_instruction(int32* orig_save_addr) {
  Unused(orig_save_addr);
  fatal("unused on aarch64 (SPARC register-window mechanism)");
  return 0;
}


void check_branch_relocation( void* fromArg, void* toArg, int32 countArg) {
  Unused(fromArg); Unused(toArg); Unused(countArg);
  fatal("aarch64 branch relocation checking not yet implemented");
}

# endif // TARGET_ARCH == AARCH64_ARCH
