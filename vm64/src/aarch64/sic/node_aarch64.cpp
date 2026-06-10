# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_node_aarch64.cpp.incl"

# ifdef SIC_COMPILER

// aarch64 SIC code generation.
//
// Every gen() entry point below is a runtime-fatal stub: the compiler's
// machine-independent passes run, but emission is not yet implemented.
// These will be filled in against the AsmJit-backed Assembler.

static void unimplemented_gen(const char* who) {
  fatal1("aarch64 SIC code generation not yet implemented: %s", who);
}

  // Frame protocol (see frame_format_aarch64.hh): the caller leaves a hole
  // at [sp+0] for the saved PC; the callee's first instruction stores lr
  // there ("store link reg here before frame creation").  Frame creation
  // then pushes the old fp, making [fp+0]=saved bp, [fp+8]=saved pc -- the
  // AAPCS64 frame record -- with the receiver at [fp+16].

  void PrologueNode::prePrologue() {
    theAssembler->Comment("save link register");
    theAssembler->str(lr, SP, leaf_pc_offset * oopSize);
  }

  void PrologueNode::postPrologue()     { }

  void BasicNode::restoreFrameAndReturn(bool haveStackFrame, fint offset) {
    Assembler* a = theAssembler;
    a->Comment("restoreFrameAndReturn");
    if (haveStackFrame) {
      a->mov(SP, fp);            // discard locals
      a->ldr(fp, SP, 0);         // restore old fp
      a->add(SP, SP, oopSize);   // pop the fp slot; sp -> saved-pc slot
    }
    a->ldr(lr, SP, leaf_pc_offset * oopSize);
    if (offset != 0)
      a->add(lr, lr, offset);    // e.g. divert to the send site's NLR entry
    a->ret();
  }

  void PrologueNode::actuallyCreateStackFrame() {
    Assembler* a = theAssembler;
    a->sub(SP, SP, oopSize);     // push old fp
    a->str(fp, SP, 0);
    a->mov(fp, SP);
    assert((thisFrameSize & (frame_word_alignment - 1)) == 0, "frame size check");
    a->sub(SP, SP, (thisFrameSize - linkage_area_size) * oopSize);

    theSIC->_frameCreationOffset = a->offset();
  }

  void PrologueNode::clearStackLocations() {
    theAssembler->Comment("clear stack locations");
    // do not have to clear outgoing args; locations covered by the 32-bit
    // register mask need no clearing either (cf. i386)
    for ( fint i = sizeof(RegisterString) * BitsPerByte;  i < theSIC->number_of_memory_locals();  ++i) {
      Location r;  int32 d;  OperandType t;
      reg_disp_type_of_loc(&r, &d, &t, StackLocation_for_index(i));
      theAssembler->str_zero(r, d);
    }
  }

  void PrologueNode::createStackFrame() {
    assert(haveStackFrame(), "shouldn't be creating a stack frame");
    thisFrameSize = theSIC->frameSize();
    actuallyCreateStackFrame();
    clearStackLocations();
  }

  void LoadIntNode::gen() {
    BasicNode::gen();
    if (isRegister(_dest->loc)) {
      theAssembler->mov_imm(_dest->loc, smi(value));
    }
    else {
      theAssembler->mov_imm(Temp2, smi(value));
      Location b;  int32 d;  OperandType t;
      reg_disp_type_of_loc(&b, &d, &t, _dest->loc);
      theAssembler->str(Temp2, b, d);
    }
  }

  void AssignNode::genOop() { unimplemented_gen("AssignNode::genOop"); }

  void BranchNode::gen() {
    BasicNode::gen();
    unimplemented_gen("BranchNode::gen");
  }

  void TBranchNode::genCompare(bool ifEqual, Location l1, Location l2) {
    Unused(ifEqual); Unused(l1); Unused(l2);
    unimplemented_gen("TBranchNode::genCompare");
  }

  void TBranchNode::testTagsIfNecessary(bool ifInt, Location l1, Location l2) {
    Unused(ifInt); Unused(l1); Unused(l2);
    unimplemented_gen("TBranchNode::testTagsIfNecessary");
  }

  bool ArrayAtNode::genAccess(Location arr, Location index, Location dest) {
    Unused(arr); Unused(index); Unused(dest);
    unimplemented_gen("ArrayAtNode::genAccess");
    return false;
  }

  bool ByteArrayAtNode::genAccess(Location arr, Location index, Location dest) {
    Unused(arr); Unused(index); Unused(dest);
    unimplemented_gen("ByteArrayAtNode::genAccess");
    return false;
  }

  bool ArrayAtPutNode::genAccess(Location arr, Location index, Location dest) {
    Unused(arr); Unused(index); Unused(dest);
    unimplemented_gen("ArrayAtPutNode::genAccess");
    return false;
  }

  bool ByteArrayAtPutNode::genAccess(Location arr, Location index, Location dest) {
    Unused(arr); Unused(index); Unused(dest);
    unimplemented_gen("ByteArrayAtPutNode::genAccess");
    return false;
  }

  void ArithRCNode::gen() {
    BasicNode::gen();
    unimplemented_gen("ArithRCNode::gen");
  }

  Location arith_genHelper(PReg* sreg, PReg* oper, PReg* dest,
                           ArithOpCode op,
                           Location& t1, Location& t2, bool& reversed) {
    Unused(sreg); Unused(oper); Unused(dest); Unused(op);
    Unused(t1); Unused(t2); Unused(reversed);
    unimplemented_gen("arith_genHelper");
    return IllegalLocation;
  }

  void BlockZapNode::gen() {
    BasicNode::gen();
    unimplemented_gen("BlockZapNode::gen");
  }

  void RestartNode::gen() {
    BasicNode::gen();
    unimplemented_gen("RestartNode::gen");
  }

  void DeadEndNode::gen() {
    BasicNode::gen();
    unimplemented_gen("DeadEndNode::gen");
  }

  void BlockCreateNode::gen() {
    BasicNode::gen();
    unimplemented_gen("BlockCreateNode::gen");
  }

  void BlockCloneNode::genCall() { unimplemented_gen("BlockCloneNode::genCall"); }

  void IndexedBranchNode::gen() {
    BasicNode::gen();
    unimplemented_gen("IndexedBranchNode::gen");
  }

  void InterruptCheckNode::gen() {
    BasicNode::gen();
    unimplemented_gen("InterruptCheckNode::gen");
  }

  void MethodReturnNode::gen() {
    BasicNode::gen();
    if (_src->isNoPReg()) {
      // control should never reach here; only happens after a non-lifo abort
      // i.e. a zapped block method. -- dmu 5/06
      theAssembler->brk(0);
      return;
    }
    // move result to ResultReg
    genHelper->moveToExactlyThisReg(_src, ResultReg);
    restoreFrameAndReturn(haveStackFrame, 0);
  }

  void NonLocalReturnNode::gen() {
    BasicNode::gen();
    restoreFrameAndReturn(true, sendDesc::non_local_return_offset);
  }

  void StoreOffsetNode::gen() {
    BasicNode::gen();
    unimplemented_gen("StoreOffsetNode::gen");
  }

  void TypeTestNode::gen() {
    BasicNode::gen();
    unimplemented_gen("TypeTestNode::gen");
  }

  void UncommonNode::gen() {
    BasicNode::gen();
    unimplemented_gen("UncommonNode::gen");
  }

  bool AbstractArrayAtNode::canCopyPropagateFrom(PReg* d) {
    Unused(d);
    unimplemented_gen("AbstractArrayAtNode::canCopyPropagateFrom");
    return false;
  }

  void AbstractArrayAtNode::markAllocated(fint* use_count, fint* def_count) {
    Unused(use_count); Unused(def_count);
    unimplemented_gen("AbstractArrayAtNode::markAllocated");
  }

  Label* ByteArrayAtPutNode::testArg2() {
    unimplemented_gen("ByteArrayAtPutNode::testArg2");
    return NULL;
  }

  void AbstractArrayAtNode::gen() {
    BasicNode::gen();
    unimplemented_gen("AbstractArrayAtNode::gen");
  }

  void PrimNode::gen() {
    BasicNode::gen();
    unimplemented_gen("PrimNode::gen");
  }

  void SendNode::gen() {
    BasicNode::gen();
    unimplemented_gen("SendNode::gen");
  }

  void BasicNode::genBranch() {
    unimplemented_gen("BasicNode::genBranch");
  }

  void FlushNode::flushRegister(PReg* r) {
    Unused(r);
    unimplemented_gen("FlushNode::flushRegister");
  }

  bool TArithRRNode::isOpInlinable(ArithOpCode o) {
    Unused(o);
    // conservatively: nothing inlinable until the aarch64 backend exists
    return false;
  }

  bool TArithRRNode::canCopyPropagateFrom(PReg* d) {
    Unused(d);
    unimplemented_gen("TArithRRNode::canCopyPropagateFrom");
    return false;
  }

  void TArithRRNode::markAllocated(fint* use_count, fint* def_count) {
    Unused(use_count); Unused(def_count);
    unimplemented_gen("TArithRRNode::markAllocated");
  }

  void TArithRRNode::gen() {
    BasicNode::gen();
    unimplemented_gen("TArithRRNode::gen");
  }

# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
