# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "sic_aarch64.hh"
# include "_sic_aarch64.cpp.incl"

# ifdef SIC_COMPILER


bool SICAllocator::keepUplevelRPRegsInMemory = true;

void SICompiler::initializeForPlatform() {
  nlrLabel = NULL;
}


int32 SICompiler::stackTempCount() {
  return number_of_memory_locals()
       + max_no_of_outgoing_args_and_rcvr();
}



fint SICompiler::max_no_of_outgoing_args_and_rcvr() {
  // +1 receiver, +1 for the lr hole at [sp+0] (BL doesn't push like CALL,
  // so the caller's frame reserves the word the callee saves lr into)
  return argCount + 2;
}


fint SICompiler::number_of_memory_locals() {
  return stackLocCount;
}

void SICompiler::check_flushability(PReg* p) {
  Unused(p);
}

void SICompiler::cope_with_uplevel_access_to(PReg* pr) {
  Unused(pr);
}



# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
