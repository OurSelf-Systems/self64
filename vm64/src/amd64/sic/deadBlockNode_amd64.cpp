# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "deadBlockNode_amd64.hh"

# include "_deadBlockNode_amd64.cpp.incl"

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
    // the current one as the prim's receiver.  The receiver slot sits at
    // [sp + rcvr_offset] here -- the call has not pushed yet (on aarch64
    // the hole word shifts it to leaf_rcvr_offset).
    Label here(theAssembler->printing);
    here.define();
    theAssembler->lea_label(Temp1, &here);
    theAssembler->store(SP, rcvr_offset * oopSize, Temp1);
    PrimNode::gen();
  }

# endif // SIC_COMPILER
# endif // TARGET_ARCH == X86_64_ARCH
