# if  TARGET_ARCH == I386_ARCH || TARGET_ARCH == X86_64_ARCH || TARGET_ARCH == AARCH64_ARCH
/* Sun-$Revision: 1.4 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "frame_iterator_amd64.hh"

# include "_frame_iterator_amd64.cpp.incl"


void FrameIterator::do_vm_frame() {
  assert(!processSemaphore, "oopClosure will not be called on resultOop in HandleReturnTrap");
# if TARGET_ARCH == I386_ARCH
  assert(SaveOutgoingArgumentsOfPatchedFrames, "always true for I386");
# endif
  if (!SaveOutgoingArgumentsOfPatchedFrames)
    return;
  // KNOWN GAP (aarch64, flag off): while a compiled send is in the VM doing
  // its lookup, the in-flight receiver and arguments live in the sender's
  // outgoing area with no GC coverage -- the callee frame that would cover
  // them via do_incoming_arguments does not exist yet, so a scavenge during
  // the lookup hands the interpreted callee stale oops at birth (richards'
  // results-check failure).  An attempt to walk them here regardless of the
  // flag fixed that but crashed on other in-flight shapes (non-send VM
  // calls, layouts outgoing_arg_count misjudges) even with the sendDesc-
  // relocation validation below; enabling this safely needs the compiler to
  // record in-flight send geometry rather than heuristics over parked pcs.
  do_incoming_arguments_of_vm_frame_called_from_self();
}


void FrameIterator::do_incoming_arguments_of_vm_frame_called_from_self() {
# if !defined(FAST_COMPILER) && !defined(SIC_COMPILER)
  // Interpreter-only: no compiled frames exist, and the send-site
  // classification below reads JIT-only nmethod state.
  return;
# else
  frame* s = f->sender();
  // Compiled senders only: an interpreted sender's return pc is VM code, not
  // a sendDesc, so send_desc()/outgoing_arg_count would read garbage -- and
  // its in-flight send values live in walked interpreter state anyway.
  if (s == NULL  ||  !s->is_compiled_self_frame())
    return;
  // Trust only genuine send sites: compiled code also calls the VM from
  // non-send positions (allocation, stack checks), where interpreting the
  // return pc as a sendDesc reads garbage.  The nmethod registers each real
  // sendDesc as a relocation entry; require the parked pc to be one of them.
  { nmethod* nm = s->code();
    if (nm == NULL) return;
    sendDesc* sd = s->send_desc();
    bool genuine = false;
    for (addrDesc* l = nm->locs(), *lend = nm->locsEnd(); l < lend; l++)
      if (l->isSendDesc() && l->asSendDesc(nm) == sd) { genuine = true; break; }
    if (!genuine) return;
  }
  // hit the outgoing args of the self frame in case it is later patched
  // (when a frame is patched we grab its outgoing args)
  fint n = s->outgoing_arg_count(f);
  for (fint i = 0;  i < n + 1 /*rcvr*/;  ++i)
    oop_closure->do_oop(
      f->location_addr_of_incoming_argument(LocationOfSavedOutgoingArgInSendee(i-1), NULL));
# endif // !FAST_COMPILER && !SIC_COMPILER
}


# if defined(FAST_COMPILER) || defined(SIC_COMPILER)
            
  void FrameIterator::do_incoming_arguments() {
    fint nargs = nm->incoming_arg_count();
    oop* p = f->first_incoming_arg_addr() - 1 /* rcvr */;
    for (fint i = -1 /* rcvr */;  i < nargs;  ++i, ++p) {
      assert(check_for_overwriting_patched_frame_saved_outgoing_args(p, i), "");
      oop_closure->do_oop(p);
    }
  }


  void FrameIterator::do_compiled() {
    nm = f->code();

    do_incoming_arguments();
    do_memory_locals();
    do_patched_frame_saved_outgoing_args();
  }
  
  
  void FrameIterator::do_memory_locals() {
    fint n = min(nm->number_of_memory_locals(),  sizeof(RegisterString) * BitsPerByte);
    oop* p = f->first_local_addr();
    fint i;
    for (i = 0;  i < n;  ++i, --p) {
      assert(check_for_overwriting_patched_frame_saved_outgoing_args(p, i), "");

           if ( isSet(mask, i) )  oop_closure->do_oop(p);
      else if ( zap            )  *p = badOop;
    }      
    # if GENERATE_DEBUGGING_AIDS
      if (CheckAssertions)
        for ( fint j = i; j  <  sizeof(RegisterString) * BitsPerByte;  ++j)  {
          assert( !isSet(mask, j), "nonsensical bit set in mask");
        }
    # endif
    
    for (  ;  i < nm->number_of_memory_locals();  ++i, --p ) {
      assert(check_for_overwriting_patched_frame_saved_outgoing_args(p, i), "");
      oop_closure->do_oop(p);
    }
  }
  

  bool FrameIterator::check_for_overwriting_patched_frame_saved_outgoing_args(oop* p, fint idx) {
    if (!f->is_patched())                                         return true;
    if (p != (oop*)f->patched_frame_saved_outgoing_args_addr(nm)) return true;
    WizardMode = true;
    lprintf("****** about to die at index %d, frame = 0x%x, next_frame = 0x%x, frame size = 0x%x (%d)\n",
            idx, f, f->sender(), f->frame_size(), f->frame_size());
    if (nm != f->code()) {
      lprintf("****** nm (0x%x) != f->code() (0x%x)\n", nm, f->code());
      nm = f->code();
    }
    if (!GCInProgress && !ScavengeInProgress) { // cannot do these when the heap is all weird
      lprintf("nmethod of offending frame is:\n");
      nm->verify();
      nm->print(); 
      lprintf("\n\n\ncode: \n");
      nm->printCode();
    }
    return false;
  }

  
# endif // either compiler
# endif // TARGET_ARCH == I386_ARCH || TARGET_ARCH == X86_64_ARCH || TARGET_ARCH == AARCH64_ARCH
