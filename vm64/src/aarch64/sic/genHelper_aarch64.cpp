# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_genHelper_aarch64.cpp.incl"

# ifdef SIC_COMPILER

// aarch64 SICGenHelper.
//
// Tag scheme (64-bit): Int_Tag=0, Mem_Tag=1, Float_Tag=2, Tag_Mask=3.
// x16 is reserved for far-branch sequences (regs_aarch64.hh), so the
// miss paths below may clobber it freely.

static void unimplemented_helper(const char* who) {
  fatal1("aarch64 SICGenHelper not yet implemented: %s", who);
}

// branch to a VM routine when the condition does NOT hold:
//   b.<cond> ok; ldr x16, =target; br x16; ok:
// (no range limit, and the pool word is patchable like any other)
static void branch_to_vm_unless(a64_cond cond, void* target) {
  Assembler* a = theAssembler;
  Label ok(a->printing);
  a->b(cond, &ok);
  a->loadAddressLiteral(x16, target, PVMAddressOperand);
  a->br(x16);
  ok.define();
}


fint SICGenHelper::spOffset(Location l) {
  Location b;  int32 d;  OperandType t;
  reg_disp_type_of_loc(&b, &d, &t, l);
  // had better be used for active frame, not a saved one,
  // because sp of saved frame is two words lower! -- dmu 5/06
  // sp sits one pad word below the i386-style frame bottom (alignment)
  return b == SP  ?  d  :  d  +  (theSIC->frameSize() - 1) * oopSize;
}

fint SICGenHelper::spOffset(Location l, nmethod* nm) {
  Location b;  int32 d;  OperandType t;
  reg_disp_type_of_loc(&b, &d, &t, l);
  // and this one is for a saved frame!
  // So, the "SP" is really going to be the fp, cause that's what blocks store -- dmu 5/06
# if GENERATE_DEBUGGING_AIDS
    if (CheckAssertions  &&  b != fp)  warning("untested");
# endif
  return b == fp  ?  d  :  d - (nm->frameSize() - 1) * oopSize;
}

// Warning: this clobbers the count register
void SICGenHelper::jumpTo(void* target, Location reg, Location link) {
  Unused(target); Unused(reg); Unused(link);
  fatal("not used on aarch64 (cf. Intel)");
}

void SICGenHelper::genCountCode(int32* counter) {
  a->Comment("count # calls");
  a->loadAddressLiteral(x16, counter, VMAddressOperand);
  a->ldr32 (x17, x16, 0);
  a->add32 (x17, x17, 1);
  a->str32 (x17, x16, 0);
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
  a->ldr(dest, src, srcOffset);
}

void SICGenHelper::store(Location src, fint dstOffset, Location dest) {
  assert(isRegister(src) && isRegister(dest), "not a register");
  a->str(src, dest, dstOffset);
}

void SICGenHelper::moveRegToReg(Location srcReg, Location destReg) {
  assert(isRegister(srcReg) && isRegister(destReg), "not a register");
  a->mov(destReg, srcReg);
}

// must be a VMAddressOperand operand
void SICGenHelper::setToZeroA(void* addr, Location tempReg) {
  a->loadAddressLiteral(tempReg, addr, VMAddressOperand);
  a->str_zero(tempReg, 0);
}

void SICGenHelper::setToZero(Location dest) {
  Location b;  int32 d;  OperandType t;
  reg_disp_type_of_loc(&b, &d, &t, dest);
  if (isRegister(dest)) a->mov_imm(dest, 0);
  else                  a->str_zero(b, d);
}


// ---- prologue type checks -------------------------------------------------
// On entry the receiver sits at [sp + leaf_rcvr_offset*oopSize]
// (frame not created yet; lr holds the return address).

void SICGenHelper::smiOop_prologue(char* missHandler) {
  a->ldr(Temp1, SP, leaf_rcvr_offset * oopSize);
  a->tst(Temp1, Tag_Mask);              // Int_Tag == 0
  branch_to_vm_unless(a64_eq, missHandler);
}

void SICGenHelper::floatOop_prologue(char* missHandler) {
  a->ldr (Temp1, SP, leaf_rcvr_offset * oopSize);
  a->andd(Temp2, Temp1, Tag_Mask);
  a->cmp (Temp2, Float_Tag);
  branch_to_vm_unless(a64_eq, missHandler);
}

void SICGenHelper::memOop_prologue(mapOop receiverMapOop, char* missHandler) {
  a->ldr (Temp1, SP, leaf_rcvr_offset * oopSize);
  a->andd(Temp2, Temp1, Tag_Mask);
  a->cmp (Temp2, Mem_Tag);
  branch_to_vm_unless(a64_eq, missHandler);
  // check_map:
  a->ldr (Temp2, Temp1, map_offset());  // tagged-pointer offset; ldur form
  a->loadOopLiteral(x16, receiverMapOop);
  a->cmp (Temp2, x16);
  branch_to_vm_unless(a64_eq, missHandler);
}


void SICGenHelper::checkOop(Label& general, oop what, Location loc_to_check) {
  // test for inline cache hit (selector, delegatee)
  Unused(general);
  moveLocToReg(loc_to_check, Temp2);
  loadImmediateOop(what, x16);
  a->cmp(Temp2, x16);
  extern char* aarch64_SendMessage_stub();
  branch_to_vm_unless(a64_eq, (void*)aarch64_SendMessage_stub());
}


// ---- dynamic-inheritance parent verification: not yet implemented ----------
// (DI sends are rare; the DI check machinery with its backpatchable nmln
// blocks ports later, alongside diDesc emission.)

fint SICGenHelper::verifyParents(objectLookupTarget* target, Location t, fint count) {
  Unused(target); Unused(t); Unused(count);
  unimplemented_helper("verifyParents (dynamic inheritance)");
  return 0;
}


void SICGenHelper::moveToExactlyThisReg(PReg* pr, Location reg) {
  Location r = moveToReg(pr, reg);
  if (r != reg) a->mov(reg, r);
}

# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
