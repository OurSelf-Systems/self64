# if defined(__x86_64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  // no pragma interface: these inlines have no out-of-line home TU,
  // and Linux Debug builds (-O0 + INTERFACE_PRAGMAS) would not emit them
# endif


# if  defined(FAST_COMPILER) || defined(SIC_COMPILER)


inline void CacheStub::jump(char* addr) {
  // target is a pooled absolute word with a CodeAddressOperand loc, so
  // the locs sequence matches getJumpLocsIndex and repatching is a store
  // (cf. cacheStub_inline_aarch64.hh; Temp2 plays x17's jump-scratch role)
  a->loadAddressLiteral(Temp2, (void*)addr, CodeAddressOperand);
  a->jmp_reg(Temp2);
}


# endif  // defined(FAST_COMPILER) || defined(SIC_COMPILER)

# endif // defined(__x86_64__)
