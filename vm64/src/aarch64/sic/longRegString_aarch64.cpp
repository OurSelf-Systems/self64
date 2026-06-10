# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "longRegString_aarch64.hh"

# include "_longRegString_aarch64.cpp.incl"


# ifdef SIC_COMPILER


void LongRegisterString::allocate(Location l) {
  doAllocate(l);
}


# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
