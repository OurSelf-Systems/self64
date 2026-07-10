# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  // no pragma interface: these inlines have no out-of-line home TU,
  // and Linux Debug builds (-O0 + INTERFACE_PRAGMAS) would not emit them
# endif



# if  defined(FAST_COMPILER) || defined(SIC_COMPILER)

  // Count stubs are not yet implemented on aarch64 (see
  // countPattern_aarch64.cpp); these accessors trap if reached.

  inline void CountStub::set_count_addr(CountCodePattern* patt, int32 addr) {
    Unused(patt); Unused(addr);
    fatal("aarch64 count stubs not yet implemented");
  }

  inline int32 CountStub::count_addr(CountCodePattern* patt) {
    Unused(patt);
    fatal("aarch64 count stubs not yet implemented");
    return 0;
  }

  inline void CountStub::set_callee(CountCodePattern* patt, int32 addr) {
    Unused(patt); Unused(addr);
    fatal("aarch64 count stubs not yet implemented");
  }



# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // defined(__aarch64__)
