# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "deadBlockNode_aarch64.hh"

# include "_deadBlockNode_aarch64.cpp.incl"

# ifdef SIC_COMPILER

  PrimDesc* DeadBlockNode::non_lifo_abort;

  void initDeadBlockNode() {
    DeadBlockNode::non_lifo_abort
      = getPrimDescOfFunction(fntype(&NLRSupport::non_lifo_abort), true);
  }

  void DeadBlockNode::gen() {
    fatal("aarch64 SIC code generation not yet implemented");
  }

# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
