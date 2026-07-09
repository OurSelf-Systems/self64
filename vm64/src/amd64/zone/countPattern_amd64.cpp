# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "countPattern_amd64.hh"
# include "_countPattern_amd64.cpp.incl"


# if  defined(FAST_COMPILER) || defined(SIC_COMPILER)

  // Count-stub code patterns are not yet designed for amd64.  The empty
  // patterns below keep countStub1_init()/countStub2_init() benign at VM
  // startup; actually instantiating a counting or comparing stub will fail
  // in CountStub creation until real patterns exist.  With the SIC as the
  // only compiler (nstages == 1), compiled code never embeds count stubs,
  // so this path is unreachable until a recompilation tier is added.

  void CountCodePattern::initCounting() {
    instsSize = 0;
    pattern = NULL;
    countAddr_offset = limit_offset = nmAddr_offset = recompileStub_offset = 0;
  }


  void CountCodePattern::initComparing() {
    instsSize = 0;
    pattern = NULL;
    countAddr_offset = limit_offset = nmAddr_offset = recompileStub_offset = 0;
  }

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == X86_64_ARCH
