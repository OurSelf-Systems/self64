# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_cacheStub_aarch64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

// PIC (polymorphic inline cache) code emission for aarch64.
//
// Entered through a send site's target word: receiver at
// [sp + leaf_rcvr_offset*oopSize], lr = the sendDesc (and must stay
// intact for the miss path).  Map constants and case targets are pooled
// absolute words; the pool is flushed at the end of finish(), so the
// loc sequence is (one word per immediate case, then map/jump pairs,
// then the fail target) exactly as getJumpLocsIndex/getMapLocsIndex
// expect.  Tag scheme: Int=0, Mem=1, Float=2.

Label* CacheStub::br_if_not_smi() {
  Label* not_smi = new Label(a->printing);
  a->tst(Temp1, Tag_Mask);
  a->b(a64_ne, not_smi);
  return not_smi;
}

Label* CacheStub::br_if_not_float() {
  Label* not_float = new Label(a->printing);
  a->andd(Temp2, Temp1, Tag_Mask);
  a->cmp(Temp2, Float_Tag);
  a->b(a64_ne, not_float);
  return not_float;
}


void CacheStub::add_case(nmethod* nm, CountStub* stArg, pc_t addr) {
  n[newMethods] = nm;  st[newMethods] = stArg;  ++ newMethods;
  jump(addr);
}


Label* CacheStub::prologue(bool immediateOnly) {
  Label* miss = NULL;
  Label* loadMapAfterHandlingImmediates = NULL;
  pc_t floatAddr, smiAddr;
  computeJumpAddr(nsmi,   theSendDesc, stsmi,   smiAddr);
  computeJumpAddr(nfloat, theSendDesc, stfloat, floatAddr);

  // put rcvr in Temp1
  a->ldr(Temp1, SP, leaf_rcvr_offset * oopSize);
  // Branch to the map-load path if the receiver is a memOop
  if (immediateOnly)
    ;
  else {
    a->andd(Temp2, Temp1, Tag_Mask);
    a->cmp(Temp2, Mem_Tag);
    loadMapAfterHandlingImmediates = new Label(a->printing);
    a->b(a64_eq, loadMapAfterHandlingImmediates);
  }

  if (nsmi  &&  nfloat) {
    Label* not_smi = br_if_not_smi();
    add_case(nsmi, stsmi, smiAddr);

    not_smi->define();
    // if immediateOnly = false and we're down here, we've already tested for
    // memOop and hence there's no need to test for the float tag.
    if (immediateOnly)
      miss = br_if_not_float();
    else
      miss = NULL;
    add_case(nfloat, stfloat, floatAddr);
  }
  else if (nsmi)   { miss = br_if_not_smi();    add_case(nsmi,   stsmi,     smiAddr);  }
  else if (nfloat) { miss = br_if_not_float();  add_case(nfloat, stfloat, floatAddr);  }
  else {
    // tested for memOop above, so rcvr is int or float,
    // but there are no smi or float cases, so must be a miss
    miss = new Label(a->printing);
    a->b(miss);
  }
  if (loadMapAfterHandlingImmediates) {
    loadMapAfterHandlingImmediates->define();
    // CacheStub::test expects Temp1 to contain the receiver's map
    a->ldr(Temp1, Temp1, map_offset());   // tagged-pointer offset; ldur form
  }
  return miss;
}


Label* CacheStub::test(oop map, pc_t addr, Label* prev) {
  if (prev)
    prev->define();
  a->loadOopLiteral(x16, map);   // pool entry order: map before its jump
  a->cmp(Temp1, x16);
  Label* next_test = new Label(a->printing);
  a->b(a64_ne, next_test);
  jump(addr);
  return next_test;
}

void CacheStub::finish(Label* miss, Label* prev) {
  if (prev)
    prev->define();
  if (miss)
    miss->define();
  a->loadAddressLiteral(x16, (void*)theSendDesc->lookupRoutine(),
                        PVMAddressOperand);
  a->br(x16);                    // lr still holds the sendDesc
  a->flushLiteralPool();         // lay the pool: locs now match the contract
}


# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == AARCH64_ARCH
