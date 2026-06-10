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
    // The lr store must NOT happen here: IC rebinds enter at
    // verifiedOffset/diCheckOffset, which are captured after this point
    // and would bypass it (on i386 the CALL pushed the return PC on
    // every path).  It lives in actuallyCreateStackFrame, where all
    // entry paths converge.
  }

  void PrologueNode::postPrologue()     { }

  // macOS arm64 enables hardware SP-alignment checking: any sp-based
  // access faults unless sp is 16-aligned.  So frames are built without
  // ever misaligning sp: the old fp is stored with a negative-offset
  // store BEFORE sp moves, and sp descends by the full (16-aligned)
  // frame size in one step, leaving one pad word at the frame bottom.
  // fp-based accesses are not alignment-checked, so fp = entry_sp - 8
  // keeps the i386/PPC frame layout intact.

  void BasicNode::restoreFrameAndReturn(bool haveStackFrame, fint offset) {
    Assembler* a = theAssembler;
    a->Comment("restoreFrameAndReturn");
    if (haveStackFrame) {
      a->add(SP, fp, oopSize);   // sp = entry sp (16-aligned before any sp access)
      a->ldr(fp, SP, -oopSize);  // old fp from [entry_sp - 8] (stur form)
    }
    a->ldr(lr, SP, leaf_pc_offset * oopSize);
    if (offset != 0)
      a->add(lr, lr, offset);    // e.g. divert to the send site's NLR entry
    a->ret();
  }

  void PrologueNode::actuallyCreateStackFrame() {
    Assembler* a = theAssembler;
    a->Comment("save link register");
    a->str(lr, SP, leaf_pc_offset * oopSize);  // all entry paths reach here
    a->str(fp, SP, -oopSize);    // save old fp at entry_sp-8 (sp unmoved)
    a->sub(fp, SP, oopSize);     // fp = entry_sp - 8
    assert((thisFrameSize & (frame_word_alignment - 1)) == 0, "frame size check");
    a->sub(SP, SP, thisFrameSize * oopSize);  // even words: sp stays 16-aligned

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

  void AssignNode::genOop() {
    ConstPReg* value = (ConstPReg*)_src;
    Location src = value->loc;
    if (src != UnAllocated) {
      // value is already in src register
      genHelper->moveRegToLoc(src, _dest->loc);
    }
    else if (isRegister(_dest->loc)) {
      genHelper->loadImmediateOop(value->constant, _dest->loc);
    }
    else {
      genHelper->loadImmediateOop(value->constant, Temp2);
      Location b;  int32 d;  OperandType t;
      reg_disp_type_of_loc(&b, &d, &t, _dest->loc);
      theAssembler->str(Temp2, b, d);
    }
  }

  static a64_cond cond_for_branch(BranchOpCode op) {
    switch (op) {
     case EQBranchOp:   return a64_eq;
     case NEBranchOp:   return a64_ne;
     case LTBranchOp:   return a64_lt;
     case LEBranchOp:   return a64_le;
     case LTUBranchOp:  return a64_cc;
     case LEUBranchOp:  return a64_ls;
     case GTBranchOp:   return a64_gt;
     case GEBranchOp:   return a64_ge;
     case GTUBranchOp:  return a64_hi;
     case GEUBranchOp:  return a64_cs;
     case VSBranchOp:   return a64_vs;
     case VCBranchOp:   return a64_vc;
     default:           ShouldNotReachHere(); return a64_al;
    }
  }

  void BranchNode::gen() {
    BasicNode::gen();
    Label* l_ = new Label(theAssembler->printing);
    if (op == ALBranchOp) theAssembler->b(l_);
    else                  theAssembler->b(cond_for_branch(op), l_);
    Node* n = next1();
    n->l = l_->unify(n->l);
  }

  // load a Location's value into a register (using t if not already in one)
  static Location to_reg(Location loc, Location t) {
    if (isRegister(loc)) return loc;
    genHelper->load(SP, genHelper->spOffset(loc), t);
    return t;
  }

  void TBranchNode::genCompare(bool haveImmediate, Location rcvrReg, Location argReg) {
    Location r = to_reg(rcvrReg, Temp1);
    if (!intRcvr) {
      // check that rcvr is a smiOop
      theAssembler->tst(r, Tag_Mask);
      Label*& primFailure = ((MergeNode*)nexti(2))->l;
      Label* l = new Label(theAssembler->printing);
      theAssembler->b(a64_ne, l);
      primFailure = primFailure->unify(l);
    }
    Location a_ = haveImmediate ? IllegalLocation : to_reg(argReg, Temp2);
    if (!intArg) {
      assert(!haveImmediate, "???");
      // check that arg is a smiOop
      theAssembler->tst(a_, Tag_Mask);
      Label*& primFailure = ((MergeNode*)nexti(2))->l;
      Label* l = new Label(theAssembler->printing);
      theAssembler->b(a64_ne, l);
      primFailure = primFailure->unify(l);
    }
    // we're here iff arg and rcvr are smiOops.  do the actual comparison
    if (haveImmediate) {
      smi val = smi(((ConstPReg*)arg)->constant);  // tagged value
      if (val >= 0 && val <= 4095) {
        theAssembler->cmp(r, (fint)val);
      } else {
        theAssembler->mov_imm(x16, val);
        theAssembler->cmp(r, x16);
      }
    } else {
      theAssembler->cmp(r, a_);
    }
  }

  void TBranchNode::testTagsIfNecessary(bool haveImmediate, Location rcvrReg, Location argReg) {
    // nothing to do: like i386, tags are checked up front in genCompare,
    // so an overflow is a real integer overflow (cf. SPARC)
    Unused(haveImmediate); Unused(rcvrReg); Unused(argReg);
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

  static void check_overflow_a64(Node* failNode) {
    if (failNode == NULL) return;
    Label* l_ = new Label(theAssembler->printing);
    theAssembler->b(a64_vs, l_);
    failNode->l = l_->unify(failNode->l);
  }

  // materialize an arith operand value (possibly an immediate) in a register
  static Location arith_operand_reg(PReg* r, Location t) {
    if (r->isConstPReg() && r->loc == UnAllocated) {
      theAssembler->mov_imm(t, smi(((ConstPReg*)r)->constant)); // tagged value
      return t;
    }
    return genHelper->moveToReg(r, t);
  }

  // Three-operand A64 arithmetic: compute s OP o into out (or just set
  // flags when out == NoReg).  Ops with overflow semantics use the
  // flag-setting forms; the caller branches on V (check_overflow_a64).
  static Location a64_arith_core(Location s, PReg* oper, ArithOpCode op,
                                 Location out, Node* failNode) {
    Unused(failNode);
    Assembler* a = theAssembler;
    bool ccOnly = (out == NoReg);
    Location dst = ccOnly ? Temp1 : out;

    // constant shift counts have a direct form
    if ((op == TARShiftCCArithOp || op == TLRShiftCCArithOp)
        && oper->isConstPReg()) {
      smi count = smiOop(((ConstPReg*)oper)->constant)->value();
      if (count < 0 || count > 61) { a->mov_imm(dst, 0); return dst; }
      if (op == TARShiftCCArithOp) a->asr(dst, s, (fint)count);
      else                         a->lsr(dst, s, (fint)count);
      a->andd(dst, dst, ~smi(Tag_Mask));    // clear dragged-in tag bits
      return dst;
    }

    Location o = arith_operand_reg(oper, x16);
    switch (op) {
     case TAddCCArithOp: case AddCCArithOp:
      a->emit32(a64_add_reg(ccOnly ? a64_xzr : dst, s, o, 0, true));  break;
     case AddArithOp:
      a->add(dst, s, o);                                              break;
     case TSubCCArithOp: case SubCCArithOp:
      a->emit32(a64_sub_reg(ccOnly ? a64_xzr : dst, s, o, 0, true));  break;
     case SubArithOp:
      a->sub(dst, s, o);                                              break;
     case TAndCCArithOp: case AndCCArithOp:
      a->emit32(a64_ands_reg(ccOnly ? a64_xzr : dst, s, o));          break;
     case AndArithOp:
      a->emit32(a64_and_reg(dst, s, o));                              break;
     case TOrCCArithOp: case OrCCArithOp: case OrArithOp:
      a->orr(dst, s, o);                                              break;
     case TXorCCArithOp: case XOrArithOp:
      a->eor(dst, s, o);                                              break;
     default:
      ShouldNotReachHere(); // op should have been rejected by isOpInlinable
    }
    return ccOnly ? NoReg : dst;
  }

  void ArithRCNode::gen() {
    BasicNode::gen();
    // reg OP raw-constant (untagged numbers; see SPrimScope::genPrimFailure)
    Assembler* a = theAssembler;
    if (_dest->isNoPReg()) {
      Location d = genHelper->moveToReg(_src, Temp1);
      switch (op) {
       case SubCCArithOp:
        if (oper >= 0 && oper <= 4095) a->cmp(d, (fint)oper);
        else { a->mov_imm(x16, oper); a->cmp(d, x16); }
        return;
       case AndCCArithOp:
        a->tst(d, oper);
        return;
       default: break;
      }
    }
    Location dest = isRegister(_dest->loc) ? _dest->loc
                  : _src->loc == Temp1 ? Temp2 : Temp1;
    Location s = genHelper->moveToReg(_src, dest);
    a->mov_imm(x16, oper);
    switch (op) {
     case AddCCArithOp:
     case AddArithOp:   a->add(dest, s, x16);                   break;
     case SubCCArithOp:
     case SubArithOp:   a->sub(dest, s, x16);                   break;
     case AndCCArithOp:
     case AndArithOp:   a->emit32(a64_and_reg(dest, s, x16));   break;
     case OrCCArithOp:
     case OrArithOp:    a->orr(dest, s, x16);                   break;
     case XOrArithOp:   a->eor(dest, s, x16);                   break;
     case ArithmeticLeftShiftArithOp:
     case LogicalLeftShiftArithOp:   a->lsl(dest, s, oper);     break;
     case ArithmeticRightShiftArithOp: a->asr(dest, s, oper);   break;
     case LogicalRightShiftArithOp:  a->lsr(dest, s, oper);     break;
     default:           ShouldNotReachHere(); // unexpected arith type
    }
    if (dest != _dest->loc) {
      a->str(dest, SP, genHelper->spOffset(_dest->loc));
    }
  }

  Location arith_genHelper(PReg* sreg, PReg* oper, PReg* dest,
                           ArithOpCode op,
                           Location& t1, Location& t2, bool& reversed) {
    Unused(t1); Unused(t2); reversed = false;
    Location s = arith_operand_reg(sreg, Temp1);
    Location out = dest->isNoPReg() ? NoReg
                 : isRegister(dest->loc) ? dest->loc : Temp1;
    return a64_arith_core(s, oper, op, out, NULL);
  }

  void BlockZapNode::gen() {
    BasicNode::gen();
    Location t = genHelper->moveToReg(block(), Temp1);
    theAssembler->mov_imm(Temp2, 0);
    theAssembler->str(Temp2, t, scope_offset());  // odd tagged offset: ldur/stur form
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
    // this node is unreachable - generate a trap for debugging
#   if GENERATE_DEBUGGING_AIDS
    if (CheckAssertions) {
      BasicNode::gen();
      theAssembler->brk(0xDEAD & 0xFFFF);
    }
#   endif
  }

  void BlockCreateNode::gen() {
    BasicNode::gen();
    if (block()->primFailBlockScope) {
      // must generate block (in primitive fail branch)
      assert(!isMemoized(), "shouldn't be memoized");
      genCall();
    } else if (isMemoized()) {
      // test if already created
      theAssembler->Comment("test memoized block");
      Location t = genHelper->moveToReg(block(), Temp1);
      genHelper->loadImmediateOop(deadBlockPR->constant, x16);
      theAssembler->cmp(t, x16);
      Label done(theAssembler->printing);
      theAssembler->b(a64_ne, &done); // optimize fast case, so predict-weird
      genCall();
      done.define();
    } else {
      // block has already been created (by initial BlockClone node)
    }
  }

  void BlockCloneNode::genCall() {
    theAssembler->Comment("block clone");
    Location dest = block()->loc;

    genHelper->loadImmediateOop(block()->block, Temp1); // load block Oop
    theAssembler->str(Temp1, SP, leaf_rcvr_offset       * oopSize);
    theAssembler->str(fp,    SP, (leaf_rcvr_offset + 1) * oopSize);
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
    Location dstBase = genHelper->moveToReg(base, Temp1);
    if (_src->isConstPReg()) {
      // store constant
      ConstPReg* value = (ConstPReg*)_src;
      oop p = value->constant;
      // don't need to check-store if oop is old - old objs will never become
      // new again
      needCheckStore = needCheckStore && p->is_new(); // ints/floats aren't new
      genHelper->loadImmediateOop(p, Temp2);
      theAssembler->str(Temp2, dstBase, offset);
    }
    else if (isRegister(_src->loc)) {
      theAssembler->str(_src->loc, dstBase, offset);
    }
    else {
      Location b;  int32 d;  OperandType tt;
      reg_disp_type_of_loc(&b, &d, &tt, _src->loc);
      theAssembler->ldr(Temp2, b, d);
      theAssembler->str(Temp2, dstBase, offset);
    }
    if (needCheckStore) {
      theAssembler->Comment("record store");
      assert(isRegister(dstBase), "base reg of check_store must be in a register");
      if (offset > card_size || !AllowOffsetCheckStores) {
        // use slow check-store sequence
        // (marked card may be off by one, but not more)
        theAssembler->add(Temp1, dstBase, offset);
        dstBase = Temp1;
      }
      if (dstBase != Temp1) theAssembler->mov(Temp1, dstBase);
      theAssembler->lsr(Temp1, Temp1, card_shift);          // card index
      theAssembler->loadAddressLiteral(x16, (void*)&byte_map_base, VMAddressOperand);
      theAssembler->ldr(x16, x16, 0);                       // byte map base
      theAssembler->add(Temp1, Temp1, x16);
      theAssembler->strb_zero(Temp1, 0);                    // mark the card
    }
  }

  void TypeTestNode::gen() {
    BasicNode::gen();
    unimplemented_gen("TypeTestNode::gen");
  }

  void UncommonNode::gen() {
    BasicNode::gen();
    genPcDesc();
    // BRK is the aarch64 "unimp"; the restart flag lives in the immediate
    // (trap-count bookkeeping comes with the deopt work)
    theAssembler->brk(restartSend ? 1 : 0);
  }

  bool AbstractArrayAtNode::canCopyPropagateFrom(PReg* d) {
    // covers AbstractArrayAtPut and both ats
    Unused(d);
    return true;
  }

  void AbstractArrayAtNode::markAllocated(fint* use_count, fint* def_count) {
    U_CHECK(_src); D_CHECK(_dest); U_CHECK(arg);
    if (error) D_CHECK(error);
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
      theAssembler->ldr(Location(x0 + i), SP, (leaf_rcvr_offset + i) * oopSize);

    emit_desc_call_head();
    Label past_nlr(theAssembler->printing);
    theAssembler->b(&past_nlr);                    // @0
    theAssembler->Data(mask());                    // @4 used registers for GC
    if (pd->needsNLRCode()) {
      nlrCode();                                   // @8
    } else {
      theAssembler->nop();                         // @8 keep the shape
    }
    theAssembler->Data((int32)0, false);           // @12 pad
    assert((theAssembler->offset() & 7) == 0, "target word must be 8-aligned");
    theAssembler->doAddOffset(PVMAddressOperand, false);
    theAssembler->DataPtr(smi(first_inst_addr(pd->fn())));  // @16
    // the shared NLR epilogue is ordinary code; it must come AFTER the
    // descriptor's fixed-offset words, never inside them
    if (pd->needsNLRCode()
        && theSIC->nlrLabel && !theSIC->nlrLabel->isDefined()) {
      theSIC->nlrLabel->define();
      restoreFrameAndReturn(true, sendDesc::non_local_return_offset);
    }
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
    extern char* aarch64_SendMessage_stub();
    theAssembler->doAddOffset(BPVMAddressOperand, false);
    theAssembler->DataPtr(smi(aarch64_SendMessage_stub()));  // @16 jump_address
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
    Label* l2 = new Label(theAssembler->printing);
    theAssembler->b(l2);
    l = l->unify(l2);
  }

  void FlushNode::flushRegister(PReg* pr) {
    // a nop on aarch64, since args always passed in memory (cf. i386)
    Unused(pr);
  }

  bool TArithRRNode::isOpInlinable(ArithOpCode op) {
    // Mul/Div/Mod and left shifts need overflow machinery that isn't
    // written yet; the SIC calls the real primitive for those instead.
    switch (op) {
     case AddArithOp:  case AddCCArithOp:  case TAddCCArithOp:
     case SubArithOp:  case SubCCArithOp:  case TSubCCArithOp:
     case AndArithOp:  case AndCCArithOp:  case TAndCCArithOp:
     case OrArithOp:   case OrCCArithOp:   case TOrCCArithOp:
     case XOrArithOp:  case TXorCCArithOp:
     case TARShiftCCArithOp:  case TLRShiftCCArithOp:
      return true;
     default:
      return false;
    }
  }

  bool TArithRRNode::canCopyPropagateFrom(PReg* d) {
    Unused(d);
    return true;  // no fixed-register ops are inlined (no idiv equivalent)
  }

  void TArithRRNode::markAllocated(fint* use_count, fint* def_count) {
    U_CHECK(_src); D_CHECK(_dest); U_CHECK(oper);
  }

  void TArithRRNode::gen() {
    // See SPrimScope::inlineIntArithmetic
    BasicNode::gen();
    if (constResult) {
      genHelper->loadImmediateOop(constResult->constant, Temp2);
      Location b;  int32 d;  OperandType t;
      reg_disp_type_of_loc(&b, &d, &t, _dest->loc);
      if (isRegister(_dest->loc)) theAssembler->mov(_dest->loc, Temp2);
      else                        theAssembler->str(Temp2, b, d);
      return;
    }
    Location s = arith_operand_reg(_src, Temp1);
    Location out = _dest->isNoPReg() ? NoReg
                 : isRegister(_dest->loc) ? _dest->loc : Temp1;
    Location dest = a64_arith_core(s, oper, op, out, next1());
    bool canOverflow = op == TAddCCArithOp || op == TSubCCArithOp;
    if (canOverflow)
      check_overflow_a64(next1());
    if (dest != NoReg && dest != _dest->loc && !_dest->isNoPReg()) {
      // store result on stack (success case)
      theAssembler->str(dest, SP, genHelper->spOffset(_dest->loc));
    }
  }

# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
