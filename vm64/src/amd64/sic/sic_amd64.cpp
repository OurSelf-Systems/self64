# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "sic_amd64.hh"
# include "_sic_amd64.cpp.incl"

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
  // +1 receiver; unlike aarch64 no extra hole word -- CALL pushes the
  // return PC below the outgoing area at send time (classic i386 scheme)
  return argCount + 1;
}


fint SICompiler::number_of_memory_locals() {
  return stackLocCount;
}

// Compile a leaf access method: map-checked slot read, constant load, or
// slot assignment.  Mirrors sic_aarch64.cpp (itself after FCompiler::
// dataCode/constantCode/assignmentCode); no frame is created and the
// stacked return PC is never disturbed, so the body simply RETs with the
// result in ResultReg.
nmethod* SICompiler::compileAccessMethod() {
  extern char* amd64_SendMessage_stub();
  char* miss = amd64_SendMessage_stub();

  theAssembler = NULL;  // may dangle from the previous compile (see initialize)
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

  // perform / dynamic-delegatee checks, as in
  // PrologueNode::doMapSelectorDelegateeChecks.  A perform site's IC gets
  // rebound to this access method like any other target; without the
  // selector compare, every later perform there with the same receiver
  // map silently hits this method regardless of selector (found via
  // browse test: the bytecode scanner's `opcodeName sendTo: bytecodes`
  // dispatched every opcode to the first-bound constant slot).
  { Label generalMiss(a->printing, NULL);
    if (L->isPerform())
      genHelper->checkOop(generalMiss, L->selector(), PerformSelectorLoc);
    if (needsDelegatee(L->lookupType()) && !L->isDelegateeStatic())
      genHelper->checkOop(generalMiss, L->delegatee(), PerformDelegateeLoc);
  }

  _diCheckOffset        = a->offset();

  // dynamic inheritance: verify assignable parents, exactly as
  // PrologueNode::verifyAssignableParents does for full methods --
  // otherwise a reassigned parent would keep hitting this stale access
  // method through the code table
  {
    fint assignableParents = L->adeps->length();
    fint count = 0;
    for (fint i = 0; i < assignableParents; ++i) {
      objectLookupTarget* target = L->adeps->start()[i];
      Location t = genHelper->loadPath(Temp2, target, LReceiverReg);
      count = genHelper->verifyParents(target, t, count);
    }
  }

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
    fint slotOffset = smiOop(dataSlot->data)->value() * oopSize - Mem_Tag;
    // the holder may be the receiver or an object reached through the
    // lookup path (e.g. an assignable slot in a shared parent)
    Location holder = genHelper->loadPath(Temp1, h, LReceiverReg);
    a->load(Temp2, SP, (leaf_rcvr_offset + 1) * oopSize);  // argument
    a->store(holder, slotOffset, Temp2);
    // check-store: mark the card for the written-to address
    a->lea(Temp1, holder, slotOffset);
    a->shr(Temp1, card_shift);
    a->loadAddressLiteral(Temp2, (void*)&byte_map_base, VMAddressOperand);
    a->load(Temp2, Temp2, 0);
    a->add(Temp1, Temp2);
    a->storeByte_zero(Temp1, 0);
    a->load(ResultReg, SP, leaf_rcvr_offset * oopSize);    // result = receiver
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
  return new_nmethod(this, generateDebugCode);
}

void SICompiler::check_flushability(PReg* p) {
  Unused(p);
}

void SICompiler::cope_with_uplevel_access_to(PReg* pr) {
  Unused(pr);
}



# endif // SIC_COMPILER
# endif // TARGET_ARCH == X86_64_ARCH
