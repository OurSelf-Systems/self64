# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "sic_aarch64.hh"
# include "_sic_aarch64.cpp.incl"

# ifdef SIC_COMPILER


bool SICAllocator::keepUplevelRPRegsInMemory = true;

void SICompiler::initializeForPlatform() {
  nlrLabel = NULL;
}


int32 SICompiler::stackTempCount() {
  return number_of_memory_locals()
       + max_no_of_outgoing_args_and_rcvr();
}



fint SICompiler::max_no_of_outgoing_args_and_rcvr() {
  // +1 receiver, +1 for the lr hole at [sp+0] (BL doesn't push like CALL,
  // so the caller's frame reserves the word the callee saves lr into)
  return argCount + 2;
}


fint SICompiler::number_of_memory_locals() {
  return stackLocCount;
}

// Compile a leaf access method: map-checked slot read, constant load, or
// slot assignment.  Mirrors FCompiler::dataCode/constantCode/assignmentCode
// (fcompiler.cpp); no frame is created and lr is never disturbed, so the
// body simply RETs with the result in ResultReg.
nmethod* SICompiler::compileAccessMethod() {
  extern char* aarch64_SendMessage_stub();
  char* miss = aarch64_SendMessage_stub();

  Assembler* a = theAssembler = new Assembler(1024, 128, PrintSICCode, true);
  genHelper = new SICGenHelper;

  stackLocCount = 0;
  argCount = (L->resultType() == assignmentResult) ? 1 : 0;

  // receiver type / map check (entry point), as in the method prologue
  oop rmap = L->receiverMapOop();
  if (rmap == Memory->smi_map->enclosing_mapOop())
    genHelper->smiOop_prologue(miss);
  else if (rmap == Memory->float_map->enclosing_mapOop())
    genHelper->floatOop_prologue(miss);
  else
    genHelper->memOop_prologue((mapOop)rmap, miss);
  _verifiedOffset       = a->offset();
  _diCheckOffset        = a->offset();
  _frameCreationOffset  = a->offset();

  MethodLookupKey* k = new_MethodLookupKey(L->key);

  switch (L->resultType()) {
   case dataResult: {
    genHelper->lookup(Temp1, L->result()->as_real(), LReceiverReg);
    a->mov(ResultReg, Temp1);
    a->ret();
    (void)rec->addDataAccessScope(k,
                                  new LocationName(LReceiverReg),
                                  L->receiverMapOop(),
                                  methodHolder_or_map());
    break;
   }
   case constantResult: {
    genHelper->loadImmediateOop(L->result()->as_real()->desc->data, ResultReg);
    a->ret();
    (void)rec->addDataAccessScope(k,
                                  new LocationName(LReceiverReg),
                                  L->receiverMapOop(),
                                  methodHolder_or_map());
    break;
   }
   case assignmentResult: {
    realSlotRef* res = L->result()->as_real();
    lookupTarget* h  = res->holder;
    slotDesc* dataSlot = h->map()->find_slot(res->desc->name);
    if (!h->is_receiver())
      fatal("unimplemented: assignment through a non-receiver holder");
    fint slotOffset = smiOop(dataSlot->data)->byte_count() - Mem_Tag;
    a->ldr(Temp1, SP, leaf_rcvr_offset * oopSize);        // receiver
    a->ldr(Temp2, SP, (leaf_rcvr_offset + 1) * oopSize);  // argument
    a->str(Temp2, Temp1, slotOffset);
    // check-store: mark the card for the written-to address
    a->add(Temp1, Temp1, slotOffset);
    a->lsr(Temp1, Temp1, card_shift);
    a->loadAddressLiteral(x16, (void*)&byte_map_base, VMAddressOperand);
    a->ldr(x16, x16, 0);
    a->add(Temp1, Temp1, x16);
    a->strb_zero(Temp1, 0);
    a->ldr(ResultReg, SP, leaf_rcvr_offset * oopSize);    // result = receiver
    a->ret();
    ScopeInfo scope = rec->addDataAssignmentScope(k,
                                  new LocationName(LReceiverReg),
                                  L->receiverMapOop(),
                                  methodHolder_or_map());
    rec->addSlot(scope, 0, new LocationName(ArgLocation(0)));
    break;
   }
   default: fatal("unexpected access kind");
  }

  a->flushLiteralPool();
  return new_nmethod(this, false);
}

void SICompiler::check_flushability(PReg* p) {
  Unused(p);
}

void SICompiler::cope_with_uplevel_access_to(PReg* pr) {
  Unused(pr);
}



# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
