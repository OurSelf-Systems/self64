/* Sun-$Revision: 30.7 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "diDesc.hh"
# include "_diDesc.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)



void DIDesc::unlink_me() {
  dependency()->remove();
  set_jump_addr(Memory->code->trapdoors->SendDIMessage_stub_td());
  MachineCache::flush_instruction_cache_for_debugging();
}

void DIDesc::print() {
  printIndent();
  lprintf("DIDesc 0x%lx\n", this);
  Indent ++;
  printIndent();
  lprintf("addr: 0x%lx; dependency: ", jump_addr());
  dependency()->print();
  lprintf("\n");
  Indent --;
}


pc_t SendDIMessage(sendDesc* sd, frame* lookupFrame, DIDesc* dc,
                    int32 verified, oop receiver, oop arg1) {
  Unused(verified);                   
  NMethodLookupKey& k =
    nmethod::findNMethod(nmethod_from_insts((pc_t)dc))->key;
  assert(k.methodHolder_or_map() != MH_TBD,
         "should be a real method holder");
  NumberOfDILookups++;
  return dc->sendMessage(lookupFrame,
                         receiver, 
                         k.selector,
                         k.delegatee,
                         sd,
                         arg1);
}


static nmethod* SendDIMessage_cont( compilingLookup* L ) {
  if ( Interpret ) {
# if TARGET_IS_64BIT
    extern fint interpreterTierUpThreshold();
    if (interpreterTierUpThreshold() > 0) {
      // Tiered mode: compile through instead of bridging every send to the
      // interpreter -- see SendMessage_cont.  NULL (uncompilable) bridges.
      nmethod* nm = L->di_desc()->lookup_compile_and_backpatch(L);
      if (nm == NULL) L->remove_all_deps();
      return nm;
    }
# endif
    L->perform_full_lookup();
    return NULL;
  }
  return L->di_desc()->lookup_compile_and_backpatch(L);
}


pc_t DIDesc::sendMessage( frame* lookupFrame,
                           oop receiver,
                           oop selector,
                           oop delegatee,
                           sendDesc* sd,
                           oop arg1 ) {
  ShowLookupInMonitor sl;
  sd->sendMessagePrologue( receiver, lookupFrame );

  ResourceMark m;
  FlushRegisterWindows(); // for vframe conversion below
  // Same GC hazard as sendDesc::sendMessage: the lookup can scavenge, and
  // receiver/selector/delegatee/arg1 live only in C locals and L's captures.
  preserved p_rcvr(receiver), p_sel(selector), p_del(delegatee), p_arg(arg1);
  // Walk the sender's outgoing receiver/argument slots across the lookup;
  // see sendDesc::sendMessage for why.
  fint out_n = selector->is_string() ? stringOop(selector)->arg_count() : 0;
  preservedArray p_out((oop*)lookupFrame + 3, 1 + out_n);
# if TARGET_IS_64BIT
  // A routed (EnterSelf-glue) caller reaches this trap too: its outgoing
  // area has send-site shape and its return point is the firstSelfFrame
  // sendDesc, so everything here works EXCEPT describing the sending frame
  // -- the glue is not an nmethod, and new_vframe would findNMethod its pc.
  // Hand the lookup a NULL sending vframe instead (block-home checks fall
  // back to last_self_frame).
  extern char* firstSelfFrame_returnPC;
  bool routed_caller = (char*)sd == firstSelfFrame_returnPC;
# else
  bool routed_caller = false;
# endif
  compilingLookup L( receiver,
                     selector,
                     delegatee,
                     MH_TBD,  // method holder
                     routed_caller ? NULL : new_vframe(lookupFrame),
                     sd,
                     this,
                     false ); // don't want a debug version

  nmethod* nm = switchToVMStack(SendDIMessage_cont,  &L);
  if (SilentTrace) LOG_EVENT1("DIDesc::sendMessage: found %#lx", nm);

# if TARGET_IS_64BIT
  if (nm == NULL) {   // uncompilable or pure-interpretation mode: bridge
    L.receiver      = p_rcvr.value;
    L.key.selector  = p_sel.value;
    L.key.delegatee = p_del.value;
    // Full mixed-mode send: marshal every argument from the caller's
    // outgoing area and handle method results.  (The arg1-only
    // interpretResultForCompiledSender bridge covers just data/constant/
    // assignment slots -- interpret_from_compiled_sender is unimplemented
    // for methods.)  Returns through ReturnResult/ReturnNLR, which end in a
    // plain return to lr and so work for compiled and glue callers alike.
    return interpretSendForCompiledSender(&L, lookupFrame);
  }
# else
  if (Interpret) {
    L.receiver      = p_rcvr.value;
    L.key.selector  = p_sel.value;
    L.key.delegatee = p_del.value;
    return L.interpretResultForCompiledSender(p_arg.value);
  }
# endif
  return nm->insts();
}


nmethod* DIDesc::lookup_compile_and_backpatch( compilingLookup* L ) {

  nmethod* nm= L->lookupNMethod();
  set_jump_addr(nm->insts());
  return nm;
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
