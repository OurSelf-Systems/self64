# if defined(__i386__) || defined(__x86_64__)
/* Sun-$Revision: 1.4 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif



// returns register string with appropriate bit set if l is a location
// described by the register mask.  Inline (cf. registerString_aarch64.hh):
// the out-of-line i386 definition lives in a FAST_COMPILER-only file that
// the SIC-only 64-bit build does not compile.
inline RegisterString registerMaskBit(Location l, fint stackLocs, fint nonRegisterArgs) {
  Unused(stackLocs); Unused(nonRegisterArgs);
  return is_StackLocation(l)  ?  nthBit(index_for_StackLocation(l)) : 0;
}
inline RegisterString regStringToMask(RegisterString s) { return s; } // no register offset on I386
inline RegisterString regMaskToString(RegisterString s) { return s; } // no register offset on I386

// dont use regs for NIC
inline Location pickRegister(RegisterString& s, RegisterString mask) { Unused(s); Unused(mask); return UnAllocated; /*return pickReverse(s, mask);*/ }


# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // defined(__i386__) || defined(__x86_64__)
