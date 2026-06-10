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

static void emit_desc_call_head();
static void gen_SPLimit_test();

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
    genPcDesc();
    gen_SPLimit_test();
    Label* dest = new Label(theAssembler->printing);
    theAssembler->b(a64_hi, dest);
    loopStart->l = loopStart->l->unify(dest);
    PrimNode::gen();
    theAssembler->b(loopStart->l);
  }

  void DeadEndNode::gen() {
    BasicNode::gen();
    unimplemented_gen("DeadEndNode::gen");
  }

  void BlockCreateNode::gen() {
    BasicNode::gen();
    unimplemented_gen("BlockCreateNode::gen");
  }

  void BlockCloneNode::genCall() {
    theAssembler->Comment("block clone");
    Location dest = block()->loc;

    genHelper->loadImmediateOop(block()->block, Temp1); // load block Oop
    theAssembler->str(Temp1, SP, rcvr_offset     * oopSize);
    theAssembler->str(fp,    SP, first_arg_offset * oopSize);
    theAssembler->mov(x0, Temp1);                       // C args: (block, fp)
    theAssembler->mov(x1, fp);

    emit_desc_call_head();
    Label past(theAssembler->printing);
    theAssembler->b(&past);                        // @0
    theAssembler->Data(mask());                    // @4
    theAssembler->nop();                           // @8
    theAssembler->Data((int32)0, false);           // @12
    theAssembler->doAddOffset(PVMAddressOperand, false);
    theAssembler->DataPtr(smi(first_inst_addr(blockClone->fn())));  // @16
    past.define();
    assert(!blockClone->needsNLRCode(), "need to rewrite this");
    genHelper->moveRegToLoc(ResultReg, dest);
  }

  void IndexedBranchNode::gen() {
    BasicNode::gen();
    unimplemented_gen("IndexedBranchNode::gen");
  }

  static void gen_SPLimit_test() {
    Assembler* a = theAssembler;
    a->Comment("stack overflow/interrupt check");
    a->loadAddressLiteral(x16, (void*)&SPLimit, VMAddressOperand);
    a->ldr(x16, x16, 0);
    a->mov(x17, SP);          // SP can't be a shifted-register cmp operand
    a->cmp(x17, x16);
  }

  void InterruptCheckNode::gen() {
    BasicNode::gen();
    genPcDesc();
    gen_SPLimit_test();
    Label l_(theAssembler->printing);
    theAssembler->b(a64_hi, &l_);  // ok: sp above limit
    PrimNode::gen();
    l_.define();
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
    assert(bci() != IllegalBCI, "should have legal bci");
    if (pd->canWalkStack()) genPcDesc();

    // Marshal the C arguments: unlike i386 (where the Self outgoing area
    // doubled as the cdecl argument list), AAPCS64 wants them in x0..x7.
    // The values stay in the outgoing area too, where the GC mask sees them.
    fint nc = argc + 1;  // receiver is C argument 0
    assert(nc <= 8, "more primitive arguments than argument registers");
    for (fint i = 0; i < nc; i++)
      theAssembler->ldr(Location(x0 + i), SP, (rcvr_offset + i) * oopSize);

    emit_desc_call_head();
    Label past_nlr(theAssembler->printing);
    theAssembler->b(&past_nlr);                    // @0
    theAssembler->Data(mask());                    // @4 used registers for GC
    if (pd->needsNLRCode()) {
      nlrCode();                                   // @8
      if (theSIC->nlrLabel && !theSIC->nlrLabel->isDefined()) {
        theSIC->nlrLabel->define();
        restoreFrameAndReturn(true, sendDesc::non_local_return_offset);
      }
    } else {
      theAssembler->nop();                         // @8 keep the shape
    }
    theAssembler->Data((int32)0, false);           // @12 pad
    assert((theAssembler->offset() & 7) == 0, "target word must be 8-aligned");
    theAssembler->doAddOffset(PVMAddressOperand, false);
    theAssembler->DataPtr(smi(first_inst_addr(pd->fn())));  // @16
    past_nlr.define();
  }

  // Call sites share one shape (see sendDesc_aarch64.hh): an 8-aligned
  // return PC followed by branch-around, mask, NLR branch, pad, and the
  // 8-byte target word at retPC+16 that the ldr/blr pair below calls
  // through.  Both real sends and primitive calls use it, so
  // sendDesc_from_addrDesc_addr works uniformly.

  static void emit_desc_call_head() {
    Assembler* a = theAssembler;
    a->align(8);                       // retPC (after blr) lands 8-aligned
    a->emit32(a64_ldr_lit(x16, 6));    // load target word at retPC+16
    a->blr(x16);
  }

  void CallNode::nlrCode() {
    theAssembler->Comment("nlrCode");
    if (nlrPoint()) {
      // branch to NLR code
      Label* l_ = new Label(theAssembler->printing);
      theAssembler->b(l_);
      nlrPoint()->l = l_->unify(nlrPoint()->l);
    }
    else {
      if (!theSIC->nlrLabel)
        theSIC->nlrLabel = new Label(theAssembler->printing);
      theAssembler->b(theSIC->nlrLabel);
    }
  }

  void SendNode::gen() {
    BasicNode::gen();
    assert(bci() != IllegalBCI, "should have legal bci");
    genPcDesc();
    genBreakpointBeforeCall();

    emit_desc_call_head();
    offset = theAssembler->offset();
    Label past_send_desc(theAssembler->printing);
    theAssembler->b(&past_send_desc);
    theAssembler->Data(mask());                    // @4
    nlrCode();                                     // @8
    theAssembler->Data((int32)0, false);           // @12 pad
    assert((theAssembler->offset() & 7) == 0, "target word must be 8-aligned");
    theAssembler->doAddOffset(BPVMAddressOperand, false);
    theAssembler->DataPtr(smi(SendMessage_stub));  // @16 jump_address word
    theAssembler->DataPtr(0);                      // @24 nmln
    theAssembler->DataPtr(0);                      // @32 nmln
    if (sel != badOop) {
      if (isPerformLookupType(l)) {
        assert_smi(sel, "should be an integer argcount");
        theAssembler->DataPtr(smiOop(sel)->value()); // @40 arg count
      } else {
        assert_string(sel, "should be a string constant");
        theAssembler->Data(sel);                     // @40 constant selector
      }
    }
    if ((l & UninlinableSendMask) == 0) theSIC->noInlinableSends = false;
    theAssembler->DataPtr(smi(l));                 // @48 lookupType
    verifySendInfo();
    if (del) {
      assert(needsDelegatee(l), "shouldn't have a delegatee");
      theAssembler->Data(del);                     // @56 delegatee
    }
    if (theSIC->nlrLabel && !theSIC->nlrLabel->isDefined()) {
      warning("untested: shared nlrLabel epilogue");
      theSIC->nlrLabel->define();
      restoreFrameAndReturn(true, sendDesc::non_local_return_offset);
    }
    past_send_desc.define();
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
