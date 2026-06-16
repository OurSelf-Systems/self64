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
    BasicNode::gen();
    genPcDesc();
    theAssembler->Comment("dead block code");
    // non_lifo_abort wants a PC inside this method as its argument; pass
    // the current one as the prim's receiver (cf. i386's call/pop trick)
    Label here(theAssembler->printing);
    here.define();
    theAssembler->adr(Temp1, &here);
    theAssembler->str(Temp1, SP, leaf_rcvr_offset * oopSize);
    PrimNode::gen();
  }

# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
