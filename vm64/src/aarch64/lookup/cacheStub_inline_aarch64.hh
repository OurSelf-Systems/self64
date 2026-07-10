# if defined(__aarch64__)

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
  a->loadAddressLiteral(x17, (void*)addr, CodeAddressOperand);
  a->br(x17);
}


# endif  // defined(FAST_COMPILER) || defined(SIC_COMPILER)

# endif // defined(__aarch64__)
