# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_node_amd64.cpp.incl"

# ifdef SIC_COMPILER

// amd64 SIC code generation, ported from node_aarch64.cpp (which mirrors
// node_i386.cpp).  x86 differences from the aarch64 file:
//  - CALL pushes the return PC, so there is no lr hole: frames are the
//    classic push rbp / mov rbp,rsp / sub rsp,N, and epilogues are
//    leave / ret (an NLR adds non_local_return_offset to the stacked
//    return PC before RET, as on i386).
//  - Arithmetic is two-address and every ALU op sets flags; pure compares
//    use cmp/test.
//  - Where aarch64 burns x16/x17 as never-allocated scratch, amd64 uses
//    Temp3/Temp4 (r14/r15; not in TempRegs, so never register-allocated),
//    and compares against pooled words directly with cmp_literal.
//  - Call sites are call [rip+16] through the 8-byte target word of the
//    descriptor laid out per sendDesc_amd64.hh (NLR entry at +5, mask at
//    +10, target word at +16).

static void emit_desc_call_head();
static void gen_SPLimit_test();

  // Frame protocol (see frame_format_amd64.hh): CALL pushes the return PC
  // on every entry path (verified/DI entries included), so unlike aarch64
  // nothing needs to be staged before frame creation.

  void PrologueNode::prePrologue()      { }

  void PrologueNode::postPrologue()     { }

  void BasicNode::restoreFrameAndReturn(bool haveStackFrame, fint offset) {
    Assembler* a = theAssembler;
    a->Comment("restoreFrameAndReturn");
    if (haveStackFrame)
      a->leave();                // rsp = rbp + ?; pops the saved rbp
    if (offset != 0)
      a->add_mem(SP, leaf_pc_offset * oopSize, offset);  // divert to NLR entry
    a->ret();
  }

  void PrologueNode::actuallyCreateStackFrame() {
    Assembler* a = theAssembler;
    a->push(FrameReg);
    a->mov(FrameReg, SP);
    assert((thisFrameSize & (frame_word_alignment - 1)) == 0, "frame size check");
    a->sub(SP, (thisFrameSize - linkage_area_size) * oopSize);

    theSIC->_frameCreationOffset = a->offset();
  }

  void PrologueNode::clearStackLocations() {
    theAssembler->Comment("clear stack locations");
    // do not have to clear outgoing args; we used to skip locals covered by
    // the register mask too, but *all* locals must be cleared or GC sees
    // stale bits and crashes (cf. the same fix in i386)
    for ( fint i = 0;  i < theSIC->number_of_memory_locals();  ++i) {
      Location r;  int32 d;  OperandType t;
      reg_disp_type_of_loc(&r, &d, &t, StackLocation_for_index(i));
      theAssembler->store_zero(r, d);
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
      theAssembler->store(b, d, Temp2);
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
      theAssembler->store(b, d, Temp2);
    }
  }

  static x64_cond cond_for_branch(BranchOpCode op) {
    switch (op) {
     case EQBranchOp:   return x64_e;
     case NEBranchOp:   return x64_ne;
     case LTBranchOp:   return x64_l;
     case LEBranchOp:   return x64_le;
     case LTUBranchOp:  return x64_b;
     case LEUBranchOp:  return x64_be;
     case GTBranchOp:   return x64_g;
     case GEBranchOp:   return x64_ge;
     case GTUBranchOp:  return x64_a;
     case GEUBranchOp:  return x64_ae;
     case VSBranchOp:   return x64_o;
     case VCBranchOp:   return x64_no;
     default:           ShouldNotReachHere(); return x64_e;
    }
  }

  void BranchNode::gen() {
    BasicNode::gen();
    Label* l_ = new Label(theAssembler->printing);
    if (op == ALBranchOp) theAssembler->jmp(l_);
    else                  theAssembler->jcc(cond_for_branch(op), l_);
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
      theAssembler->jcc(x64_ne, l);
      primFailure = primFailure->unify(l);
    }
    Location a_ = haveImmediate ? IllegalLocation : to_reg(argReg, Temp2);
    if (!intArg) {
      assert(!haveImmediate, "???");
      // check that arg is a smiOop
      theAssembler->tst(a_, Tag_Mask);
      Label*& primFailure = ((MergeNode*)nexti(2))->l;
      Label* l = new Label(theAssembler->printing);
      theAssembler->jcc(x64_ne, l);
      primFailure = primFailure->unify(l);
    }
    // we're here iff arg and rcvr are smiOops.  do the actual comparison
    if (haveImmediate) {
      smi val = smi(((ConstPReg*)arg)->constant);  // tagged value
      if (val == (smi)(int32)val) {
        theAssembler->cmp(r, (fint)val);
      } else {
        theAssembler->mov_imm(Temp2, val);         // argReg unused here
        theAssembler->cmp(r, Temp2);
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

  // Array access nodes.  A tagged smi index is value << Tag_Size (= value*4);
  // objVector elements are inline 8-byte oops, so the element byte offset is
  // index*2; byteVector data lives out of line behind the _bytes pointer.
  // dataOffset/sizeOffset already include -Mem_Tag (see the *_offset()
  // helpers in objVectorOop.hh/byteVectorOop.hh).
  // Temp3/Temp4 stand in for aarch64's x16/x17 scratch.

  bool ArrayAtNode::genAccess(Location arr, Location index, Location dest) {
    Assembler* a = theAssembler;
    a->mov(Temp4, index);
    a->shl(Temp4, 1);                   // tagged index * 2 = value * 8
    a->add(Temp4, arr);
    a->load(dest, Temp4, dataOffset);
    return true;
  }

  bool ByteArrayAtNode::genAccess(Location arr, Location index, Location dest) {
    Assembler* a = theAssembler;
    a->load(Temp4, arr, dataOffset);    // out-of-line bytes pointer
    a->mov(Temp3, index);
    a->sar(Temp3, Tag_Size);            // untagged index
    a->add(Temp4, Temp3);
    a->loadByte(dest, Temp4, 0);        // zero-extends
    a->shl(dest, Tag_Size);             // tag the result
    return true;
  }

  bool ArrayAtPutNode::genAccess(Location arr, Location index, Location dest) {
    Unused(dest);  // result is the receiver; assign it here like i386
    Assembler* a = theAssembler;
    if (_dest != _src && !_dest->isNoPReg())
      genHelper->moveRegToLoc(arr, _dest->loc);
    a->mov(Temp4, index);
    a->shl(Temp4, 1);
    a->add(Temp4, arr);                 // element address - dataOffset
    Location e = genHelper->moveToReg(elem, Temp3);
    assert(e != Temp1 && e != Temp2 && e != Temp4, "would clobber index, array or scratch");
    a->store(Temp4, dataOffset, e);
    // mark the card for the written-to element
    a->add(Temp4, dataOffset);
    a->shr(Temp4, card_shift);
    a->loadAddressLiteral(Temp2, (void*)&byte_map_base, VMAddressOperand);
    a->load(Temp2, Temp2, 0);
    a->add(Temp4, Temp2);
    a->storeByte_zero(Temp4, 0);
    return false;
  }

  bool ByteArrayAtPutNode::genAccess(Location arr, Location index, Location dest) {
    Unused(dest);  // result is the receiver; assign it here like i386
    Assembler* a = theAssembler;
    if (_dest != _src && !_dest->isNoPReg())
      genHelper->moveRegToLoc(arr, _dest->loc);
    a->load(Temp4, arr, dataOffset);    // out-of-line bytes pointer
    a->mov(Temp2, index);
    a->sar(Temp2, Tag_Size);
    a->add(Temp4, Temp2);
    Location e = genHelper->moveToReg(elem, Temp3);
    assert(e != Temp1 && e != Temp2 && e != Temp4, "would clobber index, array or scratch");
    a->mov(Temp2, e);
    a->shr(Temp2, Tag_Size);            // untag the byte value
    a->storeByte(Temp4, 0, Temp2);
    return false;                       // raw bytes: no card mark
  }

  static void check_overflow_x64(Node* failNode) {
    if (failNode == NULL) return;
    Label* l_ = new Label(theAssembler->printing);
    theAssembler->jcc(x64_o, l_);
    failNode->l = l_->unify(failNode->l);
  }

  // materialize an arith operand value (possibly an immediate) in a register
  static Location arith_operand_reg(PReg* r, Location t) {
    if (r->isConstPReg() && r->loc == UnAllocated) {
      // loadImmediateOop, not mov_imm: arith operands are usually smis, but
      // the inlined identity compares (Cmp) flow MEM OOPS through here (a
      // marker, nil, true...).  A heap oop baked into instruction immediates
      // is invisible to the GC -- no relocation entry -- so when the object
      // moved, the compare went permanently stale (this corrupted world
      // building on aarch64).  loadImmediateOop pools mem oops (GC-visible
      // word) and mov_imm's only true immediates.
      genHelper->loadImmediateOop(((ConstPReg*)r)->constant, t);
      return t;
    }
    return genHelper->moveToReg(r, t);
  }

  // Two-address x86 arithmetic: compute s OP o into out (or just set
  // flags when out == NoReg).  Every ALU op sets flags; pure compares
  // use cmp/test.  The caller branches on O (check_overflow_x64).
  static Location x64_arith_core(Location s, PReg* oper, ArithOpCode op,
                                 Location out, Node* failNode) {
    Unused(failNode);
    Assembler* a = theAssembler;
    bool ccOnly = (out == NoReg);
    Location dst = ccOnly ? Temp1 : out;

    if (op == TARShiftCCArithOp || op == TLRShiftCCArithOp) {
      if (oper->isConstPReg()) {            // constant count: direct form
        smi count = smiOop(((ConstPReg*)oper)->constant)->value();
        if (count < 0 || count > 61) { a->mov_imm(dst, 0); return dst; }
        if (dst != s) a->mov(dst, s);
        if (count != 0) {
          if (op == TARShiftCCArithOp) a->sar(dst, (fint)count);
          else                         a->shr(dst, (fint)count);
        }
      } else {                              // variable count: untag, shift
        Location o = arith_operand_reg(oper, Temp3);
        // variable shift counts live in rcx (cl); it is free outside NLRs
        a->mov(NLRHomeIDReg, o);            // rcx
        a->sar(NLRHomeIDReg, Tag_Size);     // raw shift count
        if (dst != s) a->mov(dst, s);
        if (op == TARShiftCCArithOp) a->sar_cl(dst);
        else                         a->shr_cl(dst);
      }
      a->andd(dst, ~smi(Tag_Mask));         // clear dragged-in tag bits
      return dst;
    }

    Location o = arith_operand_reg(oper, Temp3);

    if (ccOnly) {
      // flags only: use the non-destructive compares where they exist
      switch (op) {
       case TSubCCArithOp: case SubCCArithOp:
        a->cmp(s, o);                                             return NoReg;
       case TAndCCArithOp: case AndCCArithOp:
        a->tst(s, o);                                             return NoReg;
       default:
        break;      // fall through to the destructive form into Temp1
      }
    }

    if (o == dst) { a->mov(Temp4, o);  o = Temp4; }  // dst aliases the operand
    if (dst != s) a->mov(dst, s);
    switch (op) {
     case TAddCCArithOp: case AddCCArithOp: case AddArithOp:
      a->add(dst, o);                                             break;
     case TSubCCArithOp: case SubCCArithOp: case SubArithOp:
      a->sub(dst, o);                                             break;
     case TAndCCArithOp: case AndCCArithOp: case AndArithOp:
      a->andd(dst, o);                                            break;
     case TOrCCArithOp: case OrCCArithOp: case OrArithOp:
      a->orr(dst, o);                                             break;
     case TXorCCArithOp: case XOrArithOp:
      a->xorr(dst, o);                                            break;
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
        a->cmp(d, (fint)oper);
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
    if (dest != s) a->mov(dest, s);
    switch (op) {
     case AddCCArithOp:
     case AddArithOp:   a->add(dest, (fint)oper);                 break;
     case SubCCArithOp:
     case SubArithOp:   a->sub(dest, (fint)oper);                 break;
     case AndCCArithOp:
     case AndArithOp:   a->andd(dest, (smi)oper);                 break;
     case OrCCArithOp:
     case OrArithOp:    a->orr(dest, (smi)oper);                  break;
     case XOrArithOp:   a->xorr(dest, (smi)oper);                 break;
     case ArithmeticLeftShiftArithOp:
     case LogicalLeftShiftArithOp:   a->shl(dest, oper);          break;
     case ArithmeticRightShiftArithOp: a->sar(dest, oper);        break;
     case LogicalRightShiftArithOp:  a->shr(dest, oper);          break;
     default:           ShouldNotReachHere(); // unexpected arith type
    }
    if (dest != _dest->loc) {
      a->store(SP, genHelper->spOffset(_dest->loc), dest);
    }
  }

  Location arith_genHelper(PReg* sreg, PReg* oper, PReg* dest,
                           ArithOpCode op,
                           Location& t1, Location& t2, bool& reversed) {
    Unused(t1); Unused(t2); reversed = false;
    Location s = arith_operand_reg(sreg, Temp1);
    Location out = dest->isNoPReg() ? NoReg
                 : isRegister(dest->loc) ? dest->loc : Temp1;
    return x64_arith_core(s, oper, op, out, NULL);
  }

  void BlockZapNode::gen() {
    BasicNode::gen();
    Location t = genHelper->moveToReg(block(), Temp1);
    theAssembler->store_zero(t, scope_offset());  // odd tagged offset is fine
  }

  void RestartNode::gen() {
    genPcDesc();
    gen_SPLimit_test();
    Label* dest = new Label(theAssembler->printing);
    theAssembler->jcc(x64_a, dest);
    loopStart->l = loopStart->l->unify(dest);
    PrimNode::gen();
    theAssembler->jmp(loopStart->l);
  }

  void DeadEndNode::gen() {
    // this node is unreachable - generate a trap for debugging
#   if GENERATE_DEBUGGING_AIDS
    if (CheckAssertions) {
      BasicNode::gen();
      theAssembler->int3();
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
      oop marker = deadBlockPR->constant;
      if (marker->is_mem()) {
        theAssembler->cmp_literal(t, marker);
      } else {
        genHelper->loadImmediateOop(marker, Temp2);
        theAssembler->cmp(t, Temp2);
      }
      Label done(theAssembler->printing);
      theAssembler->jcc(x64_ne, &done); // optimize fast case, so predict-weird
      genCall();
      done.define();
    } else {
      // block has already been created (by initial BlockClone node)
    }
  }

  void BlockCloneNode::genCall() {
    theAssembler->Comment("block clone");
    Location dest = block()->loc;

    // C args go in registers only.  Do NOT store into the outgoing area:
    // the preallocated area may already hold a pending send's receiver/
    // arguments, and the clone would clobber them.  The proto block oop is
    // a literal-pool constant, so GC sees it without a stack copy.
    genHelper->loadImmediateOop(block()->block, Temp1); // load block Oop
    theAssembler->mov(rdi, Temp1);                      // C args: (block, fp)
    theAssembler->mov(rsi, FrameReg);

    emit_desc_call_head();
    Label past(theAssembler->printing);
    theAssembler->jmp(&past);                      // @0
    for (fint i = 0; i < 5; i++)
      theAssembler->nop();                         // @5 no NLR entry
    theAssembler->Data(mask());                    // @10
    theAssembler->Short(0);                        // @14 pad
    assert((theAssembler->offset() & 7) == 0, "target word must be 8-aligned");
    theAssembler->doAddOffset(PVMAddressOperand, false);
    theAssembler->DataPtr(smi(first_inst_addr(blockClone->fn())));  // @16
    past.define();
    assert(!blockClone->needsNLRCode(), "need to rewrite this");
    genHelper->moveRegToLoc(ResultReg, dest);
  }

  void IndexedBranchNode::gen() {
    // n-way indexed branch; fall through when non-int or out of bounds.
    // A tagged smi index is value*4; the jump table uses 8-byte slots
    // (jmp rel32 + 3 nops), so the slot byte offset is index*2.
    BasicNode::gen();
    Assembler* a = theAssembler;
    r = genHelper->moveToReg(_src, Temp1);
    Label end(a->printing);
    if (!srcMustBeSmi) {
      a->tst(r, Tag_Mask);
      a->jcc(x64_ne, &end);
    }
    smi taggedBound = smi(as_smiOop(nCases));
    a->cmp(r, (fint)taggedBound);
    a->jcc(x64_ae, &end);                // unsigned >=: out of bounds
    Location t = (r == Temp1) ? Temp2 : Temp1;
    Label jumps(a->printing);
    a->lea_label(t, &jumps);
    a->add(t, r);                        // + index*2 in two adds
    a->add(t, r);                        //   (avoids a scaled-index encoder)
    a->jmp_reg(t);
    jumps.define();
    for (fint i = 0; i < nCases; ++i) {
      Label* nthCase = new Label(a->printing);
      a->jmp(nthCase);                   // 5 bytes
      a->nop(); a->nop(); a->nop();      // pad the slot to 8
      Node* n = nexti(i + 1);
      n->l = nthCase->unify(n->l);
    }
    end.define();
  }

  static void gen_SPLimit_test() {
    Assembler* a = theAssembler;
    a->Comment("stack overflow/interrupt check");
    a->loadAddressLiteral(Temp2, (void*)&SPLimit, VMAddressOperand);
    a->load(Temp2, Temp2, 0);
    a->cmp(SP, Temp2);
  }

  void InterruptCheckNode::gen() {
    BasicNode::gen();
    genPcDesc();
    gen_SPLimit_test();
    Label l_(theAssembler->printing);
    theAssembler->jcc(x64_a, &l_);  // ok: sp above limit
    PrimNode::gen();
    l_.define();
  }

  void MethodReturnNode::gen() {
    BasicNode::gen();
    if (_src->isNoPReg()) {
      // control should never reach here; only happens after a non-lifo abort
      // i.e. a zapped block method. -- dmu 5/06
      theAssembler->int3();
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
      theAssembler->store(dstBase, offset, Temp2);
    }
    else if (isRegister(_src->loc)) {
      theAssembler->store(dstBase, offset, _src->loc);
    }
    else {
      Location b;  int32 d;  OperandType tt;
      reg_disp_type_of_loc(&b, &d, &tt, _src->loc);
      theAssembler->load(Temp2, b, d);
      theAssembler->store(dstBase, offset, Temp2);
    }
    if (needCheckStore) {
      theAssembler->Comment("record store");
      assert(isRegister(dstBase), "base reg of check_store must be in a register");
      if (offset > card_size || !AllowOffsetCheckStores) {
        // use slow check-store sequence
        // (marked card may be off by one, but not more)
        theAssembler->lea(Temp1, dstBase, offset);
        dstBase = Temp1;
      }
      if (dstBase != Temp1) theAssembler->mov(Temp1, dstBase);
      theAssembler->shr(Temp1, card_shift);                 // card index
      theAssembler->loadAddressLiteral(Temp2, (void*)&byte_map_base, VMAddressOperand);
      theAssembler->load(Temp2, Temp2, 0);                  // byte map base
      theAssembler->add(Temp1, Temp2);
      theAssembler->storeByte_zero(Temp1, 0);               // mark the card
    }
  }

  // ---- n-way type test (structure mirrors node_i386.cpp) -------------------
  // Fall-through code is the "unknown" case; define(i, l) wires case i's
  // label (0 = no match / unknown, map/oop index + 1 otherwise).
  // Receiver is in r (MapReg = Temp2); its map is loaded into
  // RcvrMapReg (= Temp1); Temp3 is the tag scratch.

  void TypeTestNode::br_if_smi(Assembler* a, Location rcvr, fint smiIndex) {
    a->tst(rcvr, Tag_Mask);                  // Int_Tag == 0
    Label* l = new Label(a->printing);
    a->jcc(x64_e, l);
    define(smiIndex, l);
  }

  void TypeTestNode::br_if_float(Assembler* a, Location rcvr, fint floatIndex) {
    a->mov(Temp3, rcvr);
    a->andd(Temp3, Tag_Mask);
    a->cmp(Temp3, Float_Tag);
    Label* l = new Label(a->printing);
    a->jcc(x64_e, l);
    define(floatIndex, l);
  }

  void TypeTestNode::br_to_unknown_case(Assembler* a) {
    Label* unknownCase = new Label(a->printing);
    a->jmp(unknownCase);
    define(0, unknownCase);
  }

  // Returns index of case to jump to, or 0 if none chosen.  Also returns
  // loadMapAfterHandlingImmediates, the label where the caller gens the map
  // load for the memOop tests. -- structure as on i386 (dmu 10/03)
  fint TypeTestNode::prologue(Assembler* a, Location rcvr, fint smiIndex,
                              fint floatIndex, bool immediateOnly,
                              Label*& loadMapAfterHandlingImmediates) {
    assert(((Float_Tag | Int_Tag) & Mem_Tag) == 0, "tagging scheme changed");
    assert(!immediateOnly || !needMapLoad,
           "immediateOnly implies !needMapLoad");

    if (!needMapLoad) {
      // no mem maps to test; rcvr could still be a memOop at this point
      if (  smiIndex)  br_if_smi  (a, rcvr,   smiIndex);
      if (floatIndex)  br_if_float(a, rcvr, floatIndex);
      return 0; // no more testing; fall through to unknown
    }

    a->tst(rcvr, Mem_Tag);                   // memOops have the low bit set
    loadMapAfterHandlingImmediates = new Label(a->printing);
    a->jcc(x64_ne, loadMapAfterHandlingImmediates);

    if (smiIndex  &&  floatIndex) {
      br_if_smi(a, rcvr, smiIndex);
      return floatIndex;
    }
    if (  smiIndex)   br_if_smi  (a, rcvr,   smiIndex);
    if (floatIndex)   br_if_float(a, rcvr, floatIndex);
    br_to_unknown_case(a);
    return 0;
  }

  void TypeTestNode::testMap(ConstPReg* pr, fint index) {
    assert(pr->constant->is_map(), "should be map");
    assert(needMapLoad, "need to load receiver map");
    theAssembler->cmp_literal(RcvrMapReg, pr->constant);
    Label* match = new Label(theAssembler->printing);
    theAssembler->jcc(x64_e, match);
    define(index, match);
  }

  void TypeTestNode::testOop(ConstPReg* pr, fint index) {
    assert(!pr->constant->is_map(), "should be oop");
    if (pr->constant->is_mem()) {
      theAssembler->cmp_literal(r, pr->constant);
    } else {
      theAssembler->mov_imm(Temp3, smi(pr->constant));
      theAssembler->cmp(r, Temp3);
    }
    Label* match = new Label(theAssembler->printing);
    theAssembler->jcc(x64_e, match);
    define(index, match);
  }

  void TypeTestNode::gen() {
    // generates n-way type test; fall-through code is "unknown" case
    BasicNode::gen();
    r = genHelper->moveToReg(_src, MapReg);

    // indexes of smi/float cases if present; one more than the maps index
    // (0 = not present)
    fint   smiIndex = 0;
    fint floatIndex = 0;
         if (maps->nth(0) == Memory->  smi_map->enclosing_mapOop())   smiIndex = 1;
    else if (maps->nth(0) == Memory->float_map->enclosing_mapOop()) floatIndex = 1;
    if (maps->length() > 1) {
           if (maps->nth(1) == Memory->  smi_map->enclosing_mapOop())   smiIndex = 2;
      else if (maps->nth(1) == Memory->float_map->enclosing_mapOop()) floatIndex = 2;
    }

    fint nconstants = 0;
    fint ntests = maps->length();
    fint firstMem = max(smiIndex, floatIndex);
    bool immediateOnly = firstMem == maps->length();

    for (fint i = firstMem; i < ntests; ++i) {
      ConstPReg* pr = mapPRs->nth(i);
      if (!pr->constant->is_map()) ++nconstants;
    }

    // first test against all constant oops
    if (!hasUnknown  &&  nconstants == ntests) {
      --ntests;       // don't need to check the last constant
    }
    for (fint i = firstMem;  i < ntests;  ++i) {
      ConstPReg* pr = mapPRs->nth(i);
      if (!pr->constant->is_map())   testOop(pr, i + 1);
    }
    if (!hasUnknown && nconstants >= ntests) {
      // last case; omit the test, branch directly
      Label* match = new Label(theAssembler->printing);
      theAssembler->jmp(match);
      define(ntests + 1, match);
      return;           // done -- tested all constants
    }

    Label* loadMapAfterHandlingImmediates = NULL;
    fint n = prologue(theAssembler, r, smiIndex, floatIndex, immediateOnly,
                      loadMapAfterHandlingImmediates);

    if (n) {
      Label* match = new Label(theAssembler->printing);
      theAssembler->jmp(match);
      define(n, match);
    }

    if (!loadMapAfterHandlingImmediates)
      ;
    else if (immediateOnly)
      define(0, loadMapAfterHandlingImmediates);     // no memOop tests
    else
      loadMapAfterHandlingImmediates->define();

    if (!hasUnknown) --ntests;      // all maps known, can omit last test
    // test against all maps
    if (needMapLoad) {
      // load receiver map (tagged-pointer offset)
      theAssembler->load(RcvrMapReg, r, map_offset());
    }
    for (fint i = firstMem; i < ntests; i++) {
      ConstPReg* pr = mapPRs->nth(i);
      if (pr->constant->is_map()) testMap(pr, i + 1);
    }
    if (!hasUnknown) {
      // last case; omit the test, branch directly
      Label* match = new Label(theAssembler->printing);
      theAssembler->jmp(match);
      define(ntests + 1, match);
    }
  }

  void UncommonNode::gen() {
    BasicNode::gen();
    genPcDesc();
    // UD2 is the x86 "unimp"; the restart flag lives in the trailing word
    // (trap-count bookkeeping comes with the deopt work)
    theAssembler->ud2();
    theAssembler->Data((int32)(restartSend ? 1 : 0), false);
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
    // the stored value must be a smi in 0..255
    Assembler* a = theAssembler;
    if (elem->isConstPReg()) {
      oop c = ((ConstPReg*)elem)->constant;
      if (c->is_smi() && smiOop(c)->value() >= 0 && smiOop(c)->value() <= 255)
        return NULL;                    // no run-time check required
      Label* L = new Label(a->printing);
      a->jmp(L);                        // primitive will always fail
      return L;
    }
    Location e = genHelper->moveToReg(elem, Temp3);
    Label* fail = new Label(a->printing);
    a->tst(e, ~(smi)(0xff << Tag_Size)); // tag bits or anything above 255
    a->jcc(x64_ne, fail);
    return fail;
  }

  void AbstractArrayAtNode::gen() {
    BasicNode::gen();
    Assembler* a = theAssembler;
    Label* argFail   = NULL;                      // arg not a smi / bad elem
    Label* indexFail = new Label(a->printing);    // index out of bounds
    Location arr   = genHelper->moveToReg(_src, Temp2);
    Location index = genHelper->moveToReg(arg, Temp1);
    if (!intArg) {
      // CP may have propagated a constant into arg
      intArg = arg->isConstPReg() && ((ConstPReg*)arg)->constant->is_smi();
    }
    if (!intArg) {
      a->tst(index, Tag_Mask);
      Label* failLabel = new Label(a->printing);
      a->jcc(x64_ne, failLabel);
      argFail = argFail->unify(failLabel);
    }
    argFail = argFail->unify(testArg2());
    // unsigned compare of tagged index against tagged length covers
    // negative indices too
    a->load(Temp4, arr, sizeOffset);
    a->cmp(index, Temp4);
    a->jcc(x64_ae, indexFail);   // unsigned >=
    Location res = isRegister(_dest->loc) ? _dest->loc : Temp1;
    bool needDestStore = genAccess(arr, index, res);
    if (needDestStore && !isRegister(_dest->loc) && !_dest->isNoPReg())
      genHelper->moveRegToLoc(res, _dest->loc);

    Label* done = new Label(a->printing);
    a->jmp(done);
    MergeNode* failMerge = (MergeNode*)next1();

    if (argFail) {
      argFail->define();
      if (error) {
        genHelper->loadImmediateOop(VMString[BADTYPEERROR], Temp4);
        genHelper->moveRegToLoc(Temp4, error->loc);
      }
      if (failMerge) {
        Label* L = new Label(a->printing);
        a->jmp(L);
        failMerge->l = failMerge->l->unify(L);
      }
    }
    indexFail->define();
    if (error) {
      genHelper->loadImmediateOop(VMString[BADINDEXERROR], Temp4);
      genHelper->moveRegToLoc(Temp4, error->loc);
    }
    if (failMerge) {
      Label* L = new Label(a->printing);
      a->jmp(L);
      failMerge->l = failMerge->l->unify(L);
    }
    done->define();
  }

  void PrimNode::gen() {
    BasicNode::gen();
    assert(bci() != IllegalBCI, "should have legal bci");
    if (pd->canWalkStack()) genPcDesc();

    // Marshal the C arguments: SysV wants the first six in rdi/rsi/rdx/
    // rcx/r8/r9 and the rest on the stack just below sp (where the CALL
    // will push the return PC beneath them).  The values stay in the
    // outgoing area too, where the GC mask sees them (frames are
    // rbp-anchored, so the temporary sp move below is invisible to GC).
    static const Location cArgRegs[6] = { rdi, rsi, rdx, rcx, r8, r9 };
    fint nc = argc + 1;  // receiver is C argument 0
    fint nstack = nc > 6 ? nc - 6 : 0;
    fint area = (nstack * oopSize + 15) & ~15;
    if (nstack) theAssembler->sub(SP, area);
    for (fint i = 0; i < (nstack ? 6 : nc); i++)
      theAssembler->load(cArgRegs[i], SP,
                         area + (rcvr_offset + i) * oopSize);
    for (fint i = 6; i < nc; i++) {
      theAssembler->load(Temp2, SP, area + (rcvr_offset + i) * oopSize);
      theAssembler->store(SP, (i - 6) * oopSize, Temp2);
    }

    emit_desc_call_head();
    Label past_nlr(theAssembler->printing);
    theAssembler->jmp(&past_nlr);                  // @0
    if (pd->needsNLRCode()) {
      nlrCode();                                   // @5 (one jmp rel32)
    } else {
      for (fint i = 0; i < 5; i++)
        theAssembler->nop();                       // @5 keep the shape
    }
    theAssembler->Data(mask());                    // @10 used registers for GC
    theAssembler->Short(0);                        // @14 pad
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
    if (nstack) theAssembler->add(SP, area);      // drop the C stack args
  }

  // Call sites share one shape (see sendDesc_amd64.hh): an 8-aligned
  // return PC followed by branch-around, NLR branch, mask, pad, and the
  // 8-byte target word at retPC+16 that the CALL below goes through.
  // Both real sends and primitive calls use it, so
  // sendDesc_from_addrDesc_addr works uniformly.

  static void emit_desc_call_head() {
    Assembler* a = theAssembler;
    a->align_end(8, 6);                // retPC (after the call) lands 8-aligned
    a->emit(x64_call_rip(16));         // call through the word at retPC+16
  }

  void CallNode::nlrCode() {
    theAssembler->Comment("nlrCode");
    if (nlrPoint()) {
      // branch to NLR code
      Label* l_ = new Label(theAssembler->printing);
      theAssembler->jmp(l_);
      nlrPoint()->l = l_->unify(nlrPoint()->l);
    }
    else {
      if (!theSIC->nlrLabel)
        theSIC->nlrLabel = new Label(theAssembler->printing);
      theAssembler->jmp(theSIC->nlrLabel);
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
    theAssembler->jmp(&past_send_desc);            // @0
    nlrCode();                                     // @5
    theAssembler->Data(mask());                    // @10
    theAssembler->Short(0);                        // @14 pad
    assert((theAssembler->offset() & 7) == 0, "target word must be 8-aligned");
    extern char* amd64_SendMessage_stub();
    theAssembler->doAddOffset(BPVMAddressOperand, false);
    theAssembler->DataPtr(smi(amd64_SendMessage_stub()));  // @16 jump_address
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
    theAssembler->jmp(l2);
    l = l->unify(l2);
  }

  void FlushNode::flushRegister(PReg* pr) {
    // a nop on amd64, since args always passed in memory (cf. i386)
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
    return true;  // no fixed-register ops are inlined (variable shifts
                  // stage their count through rcx internally)
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
      else                        theAssembler->store(b, d, Temp2);
      return;
    }
    Assembler* a = theAssembler;
    Location s = arith_operand_reg(_src, Temp1);

    // Operand smi-tag checks.  inlineIntArithmetic relies on this node to
    // route a non-smi operand to the failure merge (next1) with the Z flag
    // CLEAR, so the following BranchNode(EQBranchOp) reports a type error
    // (NE), while an arithmetic overflow arrives with Z SET (overflow, EQ).
    // Without this, a non-smi argument (e.g. a float) was silently
    // integer-added to the receiver -- the classic smi+float bug.
    bool typeErrorPossible = (!arg1IsInt || !arg2IsInt) && next1() != NULL;
    if (typeErrorPossible) {
      if (!arg1IsInt) {
        a->tst(s, Tag_Mask);
        Label* l = new Label(a->printing);
        a->jcc(x64_ne, l);               // non-smi receiver -> type error (NE)
        next1()->l = l->unify(next1()->l);
      }
      if (!arg2IsInt) {
        Location o = arith_operand_reg(oper, Temp3);  // Temp3 is never s
        a->tst(o, Tag_Mask);
        Label* l = new Label(a->printing);
        a->jcc(x64_ne, l);               // non-smi argument -> type error (NE)
        next1()->l = l->unify(next1()->l);
      }
    }

    Location out = _dest->isNoPReg() ? NoReg
                 : isRegister(_dest->loc) ? _dest->loc : Temp1;
    Location dest = x64_arith_core(s, oper, op, out, next1());
    bool canOverflow = op == TAddCCArithOp || op == TSubCCArithOp;
    if (canOverflow) {
      if (typeErrorPossible) {
        // overflow must reach the shared failure merge with Z SET so the
        // BranchNode reports overflow (EQ), not a type error
        Label cont(a->printing);
        a->jcc(x64_no, &cont);           // no overflow: continue
        a->cmp(Temp1, Temp1);            // force Z=1 (EQ), O=0
        Label* l = new Label(a->printing);
        a->jmp(l);                       // unconditional -> failure merge
        next1()->l = l->unify(next1()->l);
        cont.define();
      } else {
        check_overflow_x64(next1());
      }
    }
    if (dest != NoReg && dest != _dest->loc && !_dest->isNoPReg()) {
      // store result on stack (success case)
      a->store(SP, genHelper->spOffset(_dest->loc), dest);
    }
  }

# endif // SIC_COMPILER
# endif // TARGET_ARCH == X86_64_ARCH
