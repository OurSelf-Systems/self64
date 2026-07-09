# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation  "nmethod_amd64.hh"

# include "_nmethod_amd64.cpp.incl"

# if  defined(FAST_COMPILER) || defined(SIC_COMPILER)

void nmethod::get_platform_specific_data(AbstractCompiler* c) {
  _number_of_memory_locals               = c->number_of_memory_locals();
}

void nmethod::print_platform_specific_data() {
  ++Indent;
  lprintf( "number_of_memory_locals               = %ld\n", long(number_of_memory_locals()) );
  --Indent;
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == X86_64_ARCH
