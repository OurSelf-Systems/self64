# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "longRegString_amd64.hh"

# include "_longRegString_amd64.cpp.incl"


# ifdef SIC_COMPILER


void LongRegisterString::allocate(Location l) {
  doAllocate(l);
}


# endif // SIC_COMPILER
# endif // TARGET_ARCH == X86_64_ARCH
