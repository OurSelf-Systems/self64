# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_cacheStub_aarch64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

// PIC (polymorphic inline cache) code emission for aarch64:
// not yet implemented; see the new-sic plan.

Label* CacheStub::prologue(bool immediateOnly) {
  Unused(immediateOnly);
  fatal("aarch64 PIC emission not yet implemented");
  return NULL;
}

Label* CacheStub::test(oop what, pc_t addr, Label* prev) {
  Unused(what); Unused(addr); Unused(prev);
  fatal("aarch64 PIC emission not yet implemented");
  return NULL;
}

void CacheStub::finish(Label* miss, Label* prev) {
  Unused(miss); Unused(prev);
  fatal("aarch64 PIC emission not yet implemented");
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == AARCH64_ARCH
