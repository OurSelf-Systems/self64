# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

# ifdef INTERFACE_PRAGMAS
  // no pragma interface: these inlines have no out-of-line home TU,
  // and Linux Debug builds (-O0 + INTERFACE_PRAGMAS) would not emit them
# endif


// returns register string with appropriate bit set if l is a location
// described by the register mask.  As on i386, NumRegistersInMask == 0:
// masks describe live stack locations, not registers.
inline RegisterString registerMaskBit(Location l, fint stackLocs, fint nonRegisterArgs) {
  Unused(stackLocs); Unused(nonRegisterArgs);
  return is_StackLocation(l)  ?  nthBit(index_for_StackLocation(l)) : 0;
}

inline RegisterString regStringToMask(RegisterString s) { return s; } // no register offset on aarch64
inline RegisterString regMaskToString(RegisterString s) { return s; } // no register offset on aarch64

// don't use regs for the (nonexistent) NIC; the SIC allocates via TempRegs
inline Location pickRegister(RegisterString& s, RegisterString mask) {
  Unused(s); Unused(mask);
  return UnAllocated;
}


# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // defined(__aarch64__)
