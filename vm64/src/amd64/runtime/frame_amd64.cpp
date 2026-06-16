# if  TARGET_ARCH == I386_ARCH || TARGET_ARCH == X86_64_ARCH || TARGET_ARCH == AARCH64_ARCH
/* Sun-$Revision: 1.4 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "frame_amd64.hh"

# include "_frame_amd64.cpp.incl"


frame* frame::sender() { return my_sp()->link()->as_frame();  }

char**  frame::real_return_addr_addr() { return my_sp()->return_addr_addr();  }

char* frame::return_addr() {
  if (!InterruptedContext::the_interrupted_context->is_set())
    return platform_independent_return_addr();
    
  // Profiler uses this from interrupts, but last return address may not be right,
  // so return null.
  if ((frame*)InterruptedContext::the_interrupted_context->sp() == this)
    return NULL;
    
  // C compiler moves sp, but does not save pc for leaves
  if ( InterruptedContext::the_interrupted_context->sp()->sender()  ==  this
  &&   !Memory->code->contains(InterruptedContext::the_interrupted_context->pc()))
    return NULL;
    
  return platform_independent_return_addr();
}

  
// Return the address I was entered at, assuming I am a C++ frame

char* frame::c_entry_point() {
  frame* s = sender();
  if ( s == NULL ) return NULL;
  
  char* r = s->real_return_addr(); // where sender will return into

  if ( r == NULL || Memory->code->contains(r))
    return NULL;
# if defined(__i386__) || defined(__x86_64__)
  char* callp = r - 1;
  if (!isImmediateCall((inst_t*)callp)) return NULL;
  return  get_target_of_C_call_site((inst_t*)callp);
# elif defined(__aarch64__)
  // ARM64 BL instruction: 100101 | imm26, target = PC + sign_extend(imm26 << 2)
  uint32* blp = (uint32*)r - 1;  // instruction before return address
  uint32 insn = *blp;
  if ((insn >> 26) != 0x25)  return NULL;  // not a BL instruction
  // shift as unsigned first to avoid UB, then sign-extend
  uint32 imm26 = insn & 0x03FFFFFF;
  int64 offset = (int64)(imm26 << 2);
  if (imm26 & 0x02000000) offset |= (int64)0xFFFFFFFF00000000LL;  // sign-extend bit 27
  return (char*)blp + offset;
# else
  return NULL;
# endif
}

# pragma warn_unusedarg off

frame** frame::nmethod_frame_chain_addr(nmethod* nm) { 
  return (frame**)my_bp() + nmethod_frame_chain_offset;
}


objVectorOop* frame::patched_frame_saved_outgoing_args_addr(nmethod* nm) { 
  return (objVectorOop*)my_bp() + patched_frame_saved_outgoing_args_offset; 
}


char**  frame::currentPC_addr() { 
  return (char**)my_bp() + current_pc_offset;
}
 
 
// this may not be a self frame ????
oop*  frame::location_addr_of_incoming_argument(Location r, RegisterLocator* rl) { 
  oop* bp = (oop*)my_bp();
  if (r == IReceiverReg)
    return bp + ircvr_offset;
  assert(is_IArgLocation(r), "???");
  fint i = index_for_IArgLocation(r);
  return bp + first_iarg_offset + i;
} 

 
void**  frame::location_addr(Location r, RegisterLocator* rl) {
  Location base; int32 d; OperandType t;
  reg_disp_type_of_loc(&base, &d, &t, r);
  assert(t != RegisterOperand, "???");
# if defined(__i386__) || defined(__x86_64__)
  return (base == esp ? (void**)my_sp() : (void**)my_bp())  +  d / oopSize;
# else
  // SP-based locations (outgoing args of a pending send) are relative to the
  // frame's running sp.  Under this port's frame-object model the object
  // pointer is the callee record at running_sp - 8, so running sp = this + 8.
  // bp-based locations resolve through my_bp() = the activation's own fp.
  if (base == SP)
    return (void**)((char*)this + oopSize)  +  d / oopSize;
  return (void**)my_bp()  +  d / oopSize;
# endif
}
  

// called from recompile.c:
// copy the receiver to the new place and adjust all block homes
// (the receiver must contain references to every live block belonging
// to this frame for zapping purposes)

void frame::copy_to( char* sp,
                     char* caller,
                     char* pc,
                     bool adjust) {

  frame* new_f = (frame*)sp;
  if (SpendTimeForDebugging)
    warning("untested frame::copy_to:"); // need SIC to test this

  if (adjust) {
    // make sure all memoized blocks exist, then adjust their scope
    abstract_vframe* callee = NULL;
    OopOopTable* dummy = EMPTY;
    for ( abstract_vframe* vf = new_vframe(this);
          vf  &&  vf->fr == this;
          callee = vf,  vf = vf->sender()) {
      vf->createBlocks(callee, dummy);
    }
    ResourceMark rm; // for RegisterLocators
    adjust_blocks(block_scope_of_home_frame(), new_f, RegisterLocator::for_frame(new_f) );
  }
  copy_oops( (oop*)this, (oop*)new_f, frame_size());

  my_sp()->set_link( ((frame*)caller)->my_sp() );
  set_return_addr( pc );
}
  

// adjust fp of copied frames so they don't refer to the original stack

void frame::adjust_frame_links_of_copied_frames( frame* last_frame_to_copy,
                                                 frame* first_copied_frame) {
  
  i386_sp* osp =                     my_sp(); // this frame
  i386_sp* nsp = first_copied_frame->my_sp(); // copied frame's this
  // 64-bit: nsp (resource-area copy) and osp (process stack) can be many GB
  // apart; the difference must be pointer-width.  int32 truncated the high
  // bits, corrupting the copied frames' saved-fp links and faulting the next
  // frame walk (e.g. reading a vframe's receiver during conversion). -- rca 6/26
  fint diff = (char*)nsp - (char*)osp;
  
  while ( osp  <  last_frame_to_copy->my_sp() ) {

    nsp->adjust_link(diff);

    osp = osp->link();
    nsp = nsp->link();
  }
  // note last copied link should just be what it was so don't do anything
}

// end of copy() helpers


// From Sparc: Note that the lastFrame frame will never be returned to, so we can
// bash its registers.  But a dummy frame is needed to make the stack
// look right (to serve as the frame of the last sparc_sp).
// (used to be fixStack)


void frame::fix_frame(char* pc, char* sp) { 
  // Although conversion happens in VM process, 
  // currentProcess still says orig Self process.
  // So, unchain_frames which does Stack::first_VM_frame (sp?)
  // will go from current (conversion) frame looking for a self frame.
  // Thus, zap a conversion frame (which we never return from anyway)
  // to lead traversal back to self frame.

  assert(isOnVMStack(this), "will be overwritten");
  my_sp()->set_link((i386_sp*)sp);
  set_real_return_addr(pc);
}
 
// I386 uses bp for block home, but sp for frame*
/* Here's the deal:
   SPARC uses bp cause sp is not constant.
   This makes the VM work harder to find the block_scope (home method's frame)
   from what is stored in the block (home method's caller's frame).
   See, the home method's frame is the sp of the frame but the sp can vary, while the frame pointer cannot.
   So we store the frame pointer of the home frame in the block and crawl the stack to recover the stack pointer
   when needed by the VM.
   PPC uses the sp, which saves a lot of work.
   Intel COULD use the sp, because it doesn't change.
   BUT, at block creation time (see loadBlockOop in codeGen_i388.cpp), we have the sp BEFORE
   pushing the pc and old base pointer, so would have to subtract 8 from what we have to create the block.
   That isn't so bad, but then at NLR time, would have to compare what is in the NLRHomeReg to sp-8, would be slower.
   So for Intel, use the base pointer (frame pointer) as SPARC does.
   Could revisit this, use the sp, and just add 8 when setting the NLRHomeReg.
   Cannot use the sp+8 value in the block because the VM expects this to be a frame in various places.
   -- dmu 4/25/06
*/   

# if TARGET_IS_64BIT
// A record participates in the compiled-frame model iff its stored return
// PC is Self code: in the zone, or the EnterSelf return point (the scope
// stored by blocks of a method called straight from EnterSelf).
// Interpreter and C records are their own home frame and block scope.
static bool is_compiled_record(frame* f) {
  char* r = ((char**)f)[saved_pc_offset];
  return Memory->code->contains(r)
      || r == first_inst_addr((void*)firstSelfFrame_returnPC)
      // A patched compiled frame (return-trap / profiler / single-step) holds a
      // trap-stub address here, not its real PC; it is still a compiled frame
      // (its stashed real PC is in the zone).  Without this, a frame whose
      // return was patched gets misclassified as an interpreter/C record, so
      // home_frame_of_block_scope returns the frame itself instead of its
      // sendee -- the home of a block whose scope marker is that frame then
      // resolves to the wrong method (CodeScopeDesc::slot fatal during debug
      // block compilation).  Cannot call is_patched() here (it routes through
      // get_interpreter -> block_scope_of_home_frame -> is_compiled_record,
      // recursively), so test the trap addresses directly.  -- rca, 6/26
      || isPatchedReturnAddress(r);
}
# endif

frame* frame::block_scope_of_home_frame() {
# if TARGET_IS_64BIT
  if (!is_compiled_record(this)) return this;
# endif
   return sender();
}

frame* frame::home_frame_of_block_scope(frame* currentFrameHint) {
# if TARGET_IS_64BIT
  if (!is_compiled_record(this)) {  // see above
    Unused(currentFrameHint);
    return this;
  }
# endif
  return sendee(currentFrameHint);
}


frame* frame::home_frame_of_vfo_locals(frame* currentFrameHint) {
  return this; }

frame* frame::vfo_locals_of_home_frame() { return this; }

int32 frame::frame_size_of_uncopied_frame() { return (oop*)sender() - (oop*)this; }

char* frame::c_return_pc() {  
  return return_addr(); 
}

frame* frame::make_full_frame(char* pc)            {  
  return this; 
}
frame* frame::make_full_frame_after_trap(char* pc) {
# if TARGET_ARCH == AARCH64_ARCH
  // Self frame objects are 8 mod 16 on aarch64 (callee record at
  // running_sp - 8), not 0 mod 16 as on amd64.  Rounding to the amd64
  // parity fabricated a dummy frame handle 8 below the real one after a
  // preemption trap; vframeOops created from that handle then disagreed
  // with sender-chain walks by 8 and the watermark patch walk ran off the
  // top of the stack (stepping through compiled blocks). -- rca
  return (frame*)
       (smi(this)
    -   ((smi(this) - BytesPerWord) & (frame_word_alignment*oopSize - 1)));
# else
  return (frame*)
       (smi(this)
    -   ((smi(this) - frame_alignment_offset*oopSize) & (frame_word_alignment*oopSize - 1)));
# endif
}
frame* frame::make_full_frame_on_user_stack()      {  return this; }



void frame::print_compiled() {
  if (is_compiled_self_frame())
    lprintf(" chain = %#lx;", (nmethod_frame_chain(code())));
  lprintf("\n\tlocals = [%#lx, %#lx], currPC = %#lx, size = %ld words\n",
         (long unsigned)(my_sp()),
         (long unsigned)(currentPC()),
         long(frame_size()));
}


// 0 = first arg
// basically used for printing (tracing)
// "this" is the LOOKUP FRAME, doing the sending
oop frame::get_lookup_arg(fint index) { 
  return ((oop*)my_sp())[first_iarg_offset + index];
}


void frame::printRegs() {
}


void frame::printVerbose_on_this_platform() {
  warning("unimp intel (debugging only, anyway)");
}


oop* frame::first_incoming_arg_addr() {
  return (oop*)my_bp() + first_iarg_offset;
}

oop* frame::first_local_addr() {
  return (oop*)my_bp() + first_local_offset;
}



sendDesc* frame::send_desc() { 
  return sendDesc::sendDesc_from_return_PC( return_addr());  // it is always the same here
}



// used for scavenging, must return allocated location
//  for every interpreter state class that may exist
//  return IllegalLocation for non-interp frame

Location frame::location_of_interpreter_of_block_scope(void* entry_point) {
  return  entry_point == first_inst_addr(interpret)  
            ?  Location(IArgLocation(0))  
            :  IllegalLocation;
}


frame* frame::get_patched_self_frame(char* sp_of_patched_frame) {
  // Validated against the interpreted return-trap path (tests runAllTests):
  // sp_of_patched_frame is the patched frame itself.  Keep a WizardMode-only
  // note rather than warning on every trap.
  if (Interpret && WizardMode)
    warning("get_patched_self_frame: using sp_of_patched_frame as the frame");
# if TARGET_ARCH == AARCH64_ARCH
  // PrimCallReturnTrap's entry sp is running_sp+8 for a true prim return
  // but running_sp for a method-shaped epilogue, so the stub's sp-16 guess
  // can be one word low.  No parity rule resolves it (JIT records are
  // 8 mod 16, EnterSelf boundary records 0 mod 16); disambiguate by
  // content: only the true record's saved-pc slot still holds the patched
  // trap address. -- rca
  { frame* c = (frame*)sp_of_patched_frame;
    if (!c->is_patched()) {
      frame* c2 = (frame*)(sp_of_patched_frame + oopSize);
      if (c2->is_patched()) return c2;
    }
    return c;
  }
# else
  return (frame*)sp_of_patched_frame;
# endif
}


int32  frame::copy_through_oop_count(frame* last_frame_to_copy) {
  return (oop*)last_frame_to_copy + last_frame_to_copy->frame_size() - (oop*)this;
}


void frame::fix_current_return_address(char* ) {  }

oop frame::perform_selector_of_SendMessage_stub_frame() {
  return  ((oop*)sender()->my_sp())[PerformSelectorLoc_sp_offset / oopSize];
}  

# endif // TARGET_ARCH == I386_ARCH || TARGET_ARCH == X86_64_ARCH || TARGET_ARCH == AARCH64_ARCH
