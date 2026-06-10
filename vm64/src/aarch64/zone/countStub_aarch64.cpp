# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_countStub_aarch64.cpp.incl"

# if  defined(FAST_COMPILER) || defined(SIC_COMPILER)


  void AgingStub::initPattern() {
    // nothing to do (as on i386)
  }


  pc_t CountStub::jump_addr() {
    fatal("aarch64 count stubs not yet implemented");
    return NULL;
  }

  void ComparingStub::init(nmethod* nm) {
    Unused(nm);
    fatal("aarch64 count stubs not yet implemented");
  }

  void AgingStub::init(nmethod* nm) {
    Unused(nm);
    fatal("aarch64 count stubs not yet implemented");
  }

  void ComparingStub::set_recompile_addr(pc_t addr) {
    Unused(addr);
    fatal("aarch64 count stubs not yet implemented");
  }


# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)

# endif // TARGET_ARCH == AARCH64_ARCH
