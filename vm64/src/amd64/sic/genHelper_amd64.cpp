# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_genHelper_amd64.cpp.incl"

# ifdef SIC_COMPILER

// amd64 SICGenHelper (cf. genHelper_aarch64.cpp).
//
// Tag scheme (64-bit): Int_Tag=0, Mem_Tag=1, Float_Tag=2, Tag_Mask=3.
// Where aarch64 burns x16/x17 on far-branch and compare scratch, amd64
// jumps and compares straight through literal-pool words (jmp_literal,
// cmp_literal), so most sequences here need no scratch register at all.

// branch to a VM routine when the condition does NOT hold:
//   j<cond> ok; jmp [pool word = target]; ok:
static void branch_to_vm_unless(x64_cond cond, void* target) {
  Assembler* a = theAssembler;
  Label ok(a->printing);
  a->jcc(cond, &ok);
  a->jmp_literal(target, PVMAddressOperand);
  ok.define();
}


fint SICGenHelper::spOffset(Location l) {
  Location b;  int32 d;  OperandType t;
  reg_disp_type_of_loc(&b, &d, &t, l);
  // had better be used for active frame, not a saved one,
  // because sp of saved frame is two words lower! -- dmu 5/06
  return b == SP  ?  d  :  d  +  (theSIC->frameSize() - linkage_area_size) * oopSize;
}

fint SICGenHelper::spOffset(Location l, nmethod* nm) {
  Location b;  int32 d;  OperandType t;
  reg_disp_type_of_loc(&b, &d, &t, l);
  // and this one is for a saved frame!
  // So, the "SP" is really going to be the fp, cause that's what blocks store -- dmu 5/06
# if GENERATE_DEBUGGING_AIDS
    if (CheckAssertions  &&  b != FrameReg)  warning("untested");
# endif
  return b == FrameReg  ?  d  :  d - (nm->frameSize() - linkage_area_size) * oopSize;
}

// Warning: this clobbers the count register
void SICGenHelper::jumpTo(void* target, Location reg, Location link) {
  Unused(target); Unused(reg); Unused(link);
  fatal("not used for Intel");
}

void SICGenHelper::genCountCode(int32* counter) {
  a->Comment("count # calls");
  a->loadAddressLiteral(Temp2, counter, VMAddressOperand);
  a->add32_mem(Temp2, 0, 1);
}


Location SICGenHelper::loadImmediateOop(ConstPReg* r, Location dest, bool mustMove) {
  // load oop from ConstPR; return location containing the oop
  if (r->loc == UnAllocated) {
    loadImmediateOop(r->constant, dest);
    return dest;
  } else if (mustMove) {
    warning("untested: loadImmediateOop with mustMove");
    moveRegToReg(r->loc, dest);
    return dest;
  }
  else
    return r->loc;
}

void SICGenHelper::loadImmediateOop(oop p, Location dest, bool isInt) {
  Unused(isInt);
  assert(isRegister(dest), "must be a register");
  if (p->is_mem()) {
    a->loadOopLiteral(dest, p);     // GC-visible pool word
  } else {
    a->mov_imm(dest, smi(p));       // immediates (smis, floats) never move
  }
}

void SICGenHelper::load(Location src, fint srcOffset, Location dest) {
  assert(isRegister(src) && isRegister(dest), "not a register");
  a->load(dest, src, srcOffset);
}

void SICGenHelper::store(Location src, fint dstOffset, Location dest) {
  assert(isRegister(src) && isRegister(dest), "not a register");
  a->store(dest, dstOffset, src);
}

void SICGenHelper::moveRegToReg(Location srcReg, Location destReg) {
  assert(isRegister(srcReg) && isRegister(destReg), "not a register");
  a->mov(destReg, srcReg);
}

// must be a VMAddressOperand operand
void SICGenHelper::setToZeroA(void* addr, Location tempReg) {
  // 32-bit store: the only client is the LRU unused-bit reset, and
  // LRUflag is an int32 array doubling as the IDManager free list -- an
  // 8-byte store would wipe the next ID's free-list link too
  a->loadAddressLiteral(tempReg, addr, VMAddressOperand);
  a->store_zero32(tempReg, 0);
}

void SICGenHelper::setToZero(Location dest) {
  Location b;  int32 d;  OperandType t;
  reg_disp_type_of_loc(&b, &d, &t, dest);
  if (isRegister(dest)) a->mov_imm(dest, 0);
  else                  a->store_zero(b, d);
}


// ---- prologue type checks -------------------------------------------------
// On entry the receiver sits at [rsp + leaf_rcvr_offset*oopSize]
// (frame not created yet; [rsp] holds the return address).

void SICGenHelper::smiOop_prologue(char* missHandler) {
  a->load(Temp1, SP, leaf_rcvr_offset * oopSize);
  a->tst(Temp1, Tag_Mask);              // Int_Tag == 0
  branch_to_vm_unless(x64_e, missHandler);
}

void SICGenHelper::floatOop_prologue(char* missHandler) {
  a->load(Temp1, SP, leaf_rcvr_offset * oopSize);
  a->mov (Temp2, Temp1);
  a->andd(Temp2, Tag_Mask);
  a->cmp (Temp2, Float_Tag);
  branch_to_vm_unless(x64_e, missHandler);
}

void SICGenHelper::memOop_prologue(mapOop receiverMapOop, char* missHandler) {
  a->load(Temp1, SP, leaf_rcvr_offset * oopSize);
  a->mov (Temp2, Temp1);
  a->andd(Temp2, Tag_Mask);
  a->cmp (Temp2, Mem_Tag);
  branch_to_vm_unless(x64_e, missHandler);
  // check_map:
  a->load(Temp2, Temp1, map_offset());  // tagged-pointer offset
  a->cmp_literal(Temp2, receiverMapOop);
  branch_to_vm_unless(x64_e, missHandler);
}


void SICGenHelper::checkOop(Label& general, oop what, Location loc_to_check) {
  // test for inline cache hit (selector, delegatee)
  Unused(general);
  moveLocToReg(loc_to_check, Temp2);
  if (what->is_mem()) {
    a->cmp_literal(Temp2, what);
  } else {
    a->mov_imm(Temp1, smi(what));
    a->cmp(Temp2, Temp1);
  }
  extern char* amd64_SendMessage_stub();
  branch_to_vm_unless(x64_e, (void*)amd64_SendMessage_stub());
}


// ---- dynamic-inheritance parent verification --------------------------------
// The prologue must verify that each assignable parent still holds the value
// (or map) the method was compiled for.  On a mismatch we jump through a
// patchable DI desc (diDesc_amd64.hh) whose target is initially
// SendDIMessage_stub.

fint SICGenHelper::verifyParents(objectLookupTarget* target, Location t, fint count) {
  assert(target->links != 0, "expecting an assignable parent link");
  bool isFirst = true;
  for (assignableSlotLink* l = target->links;
       l != 0;
       l = l->next, isFirst = false) {

    if (!isFirst) {
      // if multiple dynamic parents, reload slot holder before looping (HACK!)
      t = loadPath(Temp1, target, LReceiverReg);
    }

    // load assignable parent slot value
    load(t, smiOop(l->slot->data)->value() * oopSize - Mem_Tag, Temp1);
    verifyOneImmediateParent(l, Temp1, Temp2, count);
    ++count;

    if (l->target->links) count = verifyParents(l->target, Temp1, count);
  }
  return count;
}


void SICGenHelper::verifyOneImmediateParent(assignableSlotLink* l,
                                            Location parentOopReg,
                                            Location scratchReg,
                                            fint count) {
  Label* ok = new Label(a->printing);

  if (l->target->value_constrained)
    verifyConstrainedOopOfParent(l->target->obj, parentOopReg, ok);
  else
    verifyMapOfParent(l->target->obj->map(), parentOopReg, scratchReg, ok);

  // Miss: jump through a backpatchable DI desc.  Pass the number of parents
  // verified so far and the desc's reference point (the target word).
  a->Comment("DI parent miss");
  a->mov_imm(DICountReg, count);
  Label desc_ref(a->printing);
  a->lea_label(DIInlineCacheReg, &desc_ref);
  a->align_end(8, 6);                // desc_ref (just past the jmp) lands 8-aligned
  a->emit(x64_jmp_rip(0));           // jmp [rip+0] -- the target word below
  desc_ref.define();
  a->doAddOffset(DIVMAddressOperand, false);
  a->DataPtr(smi(Memory->code->trapdoors->SendDIMessage_stub_td()));
  a->DataPtr(0);                     // nmln next (init'd at nmethod install)
  a->DataPtr(0);                     // nmln prev

  ok->define();
}


void SICGenHelper::verifyConstrainedOopOfParent(oop targetOop,
                                                Location parentOopReg,
                                                Label* ok) {
  // constraint for a particular oop (ambiguity resolution)
  if (targetOop->is_mem()) {
    a->cmp_literal(parentOopReg, targetOop);
  } else {
    a->mov_imm(Temp2, smi(targetOop));
    a->cmp(parentOopReg, Temp2);
  }
  a->jcc(x64_e, ok);
}


// Given: map to look for, obj already in parentOopReg, scratch reg regForMap.
// Test for map, fall through on miss, goto label ok on hit.
void SICGenHelper::verifyMapOfParent(Map* targetMap,
                                     Location parentOopReg,
                                     Location regForMap,
                                     Label* ok) {
  if (targetMap == Memory->smi_map) {
    a->tst(parentOopReg, Tag_Mask);            // Int_Tag == 0
    a->jcc(x64_e, ok);
  } else if (targetMap == Memory->float_map) {
    a->mov (regForMap, parentOopReg);
    a->andd(regForMap, Tag_Mask);
    a->cmp (regForMap, Float_Tag);
    a->jcc(x64_e, ok);
  } else {
    Label miss(a->printing);
    a->mov (regForMap, parentOopReg);
    a->andd(regForMap, Tag_Mask);
    a->cmp (regForMap, Mem_Tag);
    a->jcc(x64_ne, &miss);                     // not a mem oop
    a->load(regForMap, parentOopReg, map_offset());
    a->cmp_literal(regForMap, targetMap->enclosing_mapOop());
    a->jcc(x64_e, ok);
    miss.define();
  }
}


void SICGenHelper::moveToExactlyThisReg(PReg* pr, Location reg) {
  Location r = moveToReg(pr, reg);
  if (r != reg) a->mov(reg, r);
}

# endif // SIC_COMPILER
# endif // TARGET_ARCH == X86_64_ARCH
