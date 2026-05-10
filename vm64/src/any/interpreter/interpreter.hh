/* Sun-$Revision: 30.9 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


extern oop sneaky_method_argument_to_interpret;

extern "C" {
  oop interpret( oop rcv,
                 oop sel,
                 oop del,
                 oop meth,
                 oop _mh,
                 oop* _args,
                 int32 _nargs );
  oop interpret_from_compiled_sender();
}


enum ReturnPatchReason { not_patched, patched_for_profiling, patched };


static const int PIC_SIZE = 4;

struct PICEntry {
  mapOop  cachedMap;
  oop     cachedMethod;
  oop     cachedHolder;   // NULL means "use current receiver" on PIC hit
};

struct InterpreterPIC {
  PICEntry entries[PIC_SIZE];
  int8_t   count;
  int8_t   next;
  // Data send caching: per-entry result type and slot offset.
  // Enables short-circuiting data/constant sends at PIC hit time
  // without full lookup or interpret() call.
  // Values: 0=methodResult, 1=constantResult, 2=dataResult, 3=assignmentResult
  int8_t   resultType[PIC_SIZE];
  int32    slotOffset[PIC_SIZE]; // obj_slot offset for data/assignment results
};

extern void invalidate_all_interpreter_pics();


class interpreter: public abstract_interpreter {
  friend class interpreted_vframe; // needs mi.literals, etc
  friend class InterpreterIterator;

 public:

  // WARNING all oops here must appear in ITERATOR below
  oop receiver;
  // next 2 are not redundant because of performs:
  oop selector;
  oop delegatee;
  oop method_object; 
  
 protected:
  oop _methodHolder;
  
 public:
  oop* args;
  int32 length_args;

  oop* locals;
  int32 _length_locals;
  smi minOffset;
  bool hasParentLocalSlot;
  oop* cloned_blocks; // NULL or cloned block
  oop* stack;
  int32 sp;
  oop self;

  // put next two in here instead of locals so that
  //  they will get scavenged; they have to survive a send
  //  when a prim fails
        oop rcvToSend;
  oop       selToSend; // could be anything if a perform
  int32     arg_count; // for the send, not for me
  
  // unlilke compiled frames, cannot patch the return address
  //  because that would lose the call used to verify that this is
  //  an interpreted frame. Instead, save the address that WOULD
  //  be installed here, and check it upon return.

  ReturnPatchReason  return_patch_reason;
  bool               restartSend;
  
  interpreter* parentI; // interp of lexical parent or NULL

  // On x86_64 (interpreter-only, no JIT), frame-based interpreter
  // lookup doesn't work (arguments are in registers, not on the stack).
  // Maintain a linked list of active interpreters so we can find
  // the interpreter for a given frame pointer.
  frame*        _my_frame;      // frame ptr of our interpret() C function
  interpreter*  _prev_interp;   // linked list of active interpreters
  static interpreter* _active_interp_list;

  static interpreter* find_interpreter_for_frame(frame* f);
 
  InterpreterPIC* _pics;       // points into InterpreterPICTable (heap) or NULL
  int32           _num_pics;
  int16_t*        _pc_to_pic;  // points into InterpreterPICTable (heap) or NULL

  // Pointer to the in-progress simpleLookup on this interpreter activation,
  // or NULL when no lookup is active.
  //
  // When a scavenge can happen during a lookup:
  //   interpreter::lookup_and_send constructs `simpleLookup L` on the C
  //   stack and then calls switchToVMStack_intSend(&L, ...). The lookup
  //   itself may invoke primitives, recurse into interpret() to run user
  //   code, and allocate — and any allocation can trigger a scavenge. So
  //   between L's construction and the post-call use of L's fields
  //   (L.evaluateResult, PIC cache fill), an arbitrary number of scavenges
  //   may fire.
  //
  // Why that's a problem:
  //   L's constructor captures the current `receiver`, `selector`,
  //   `delegatee`, the receiver's mapOop, and `methodHolder_or_map` — all
  //   heap pointers — into L's fields. The scavenger walks the heap and
  //   the Self stacks but does NOT walk the C stack. So L's captures go
  //   stale across the call: their pointees have been relocated, but L
  //   still holds the pre-move addresses. Subsequent reads (evaluateResult,
  //   PIC fill) see stale data and dispatch the wrong method, leaving
  //   is.argument_count inconsistent with the real selector — surfacing
  //   later as the "argument_count N != selector's arg_count M" fatal.
  //
  // The fix:
  //   Stash &L here for the duration of the call. InterpreterIterator,
  //   which already walks this interpreter's oops on every scavenge, also
  //   walks L's captures via L->oops_do(closure) when this field is
  //   non-NULL. The captures are then updated in place.
  class simpleLookup* lookup_in_progress;

  // Setter that asserts the no-re-entrancy invariant: each interpreter
  // activation has at most one lookup in progress at a time. Nested sends
  // create new interpreter activations (each with its own
  // lookup_in_progress), so re-entrancy on the same interp would indicate
  // a structural change we'd need to handle (e.g. switching to a chain).
  inline void set_lookup_in_progress(class simpleLookup* L) {
    assert(lookup_in_progress == NULL,
           "re-entrant lookup — previous one should have been cleared");
    lookup_in_progress = L;
  }

# if TARGET_IS_64BIT
  // On x86_64 interpreter-only builds, ContinueNLRFromC and c_entry_point()
  // don't work.  Use setjmp/longjmp for NLR unwinding instead.
  jmp_buf       _nlr_jmpbuf;

  static interpreter* active_interp();
  jmp_buf&      nlr_jmpbuf()          { return _nlr_jmpbuf; }
# endif

 protected:
   frame* _block_scope_or_NLR_target;

   inline frame* block_scope_or_NLR_target();

 public:
  interpreter( oop rcv,
               oop sel,
               oop del,
               oop meth,
               oop _mh,
               oop* _args,
               int32 _nargs);

  oop top() { return stack[sp-1]; }
  
  inline int32 length_cloned_blocks();
  inline void     set_cloned_blocks(void* p);

  inline int32    length_stack() { return mi.length_codes; }
  inline void     set_stack(void* p);

  inline int32 length_locals() { return _length_locals; }
  inline void  set_locals(void* p);

               
  PrimDesc* current_primDesc; // for stack walking
  
  PrimDesc* getPrimDesc() { return current_primDesc; } 

  void set_restartSend(bool b) {restartSend = b;}

  int32 num_pics() { return _num_pics; }
  void  attach_pics();  // look up or create PICs in the persistent table

  void interpret_method();

  frame* my_frame() {return _my_frame; }


  oop methodHolder() {
    assert( ! _methodHolder->is_map(), "interp mh always object");
    return _methodHolder;
  }
  
  void set_methodHolder(oop mh) { _methodHolder= mh; }

  // frame oop iterators need this one (via INTERPRETER_ITERATOR below):

  oop* methodHolder_addr() { return &_methodHolder; }
  
  abstract_vframe* parentVF();
  
 protected: 
 
  void do_SELF_CODE();
  void do_POP_CODE();
  void do_NONLOCAL_RETURN_CODE();
  void do_literal_code(oop lit);
  void do_read_write_local_code(bool isWrite);
  void do_send_code(bool isSelfImplicit, stringOop selector, fint arg_count);
  
  void do_branch_code( int32 target_PC, oop target_oop = badOop );
  void do_BRANCH_INDEXED_CODE();

  
  
 public:
  void send( LookupType, oop delegatee, fint arg_count );
  oop lookup_and_send( LookupType,
                       oop mh,
                       oop delegatee);
                      
  oop send_prim( );

private:
  oop try_pic(LookupType, oop delOrNameToSend, int32 resSP);
  // If pic entry i matches rMap, produce its result, update the stack, and
  // return true; otherwise return false.  -- claude & dmu May 2026
  oop try_pic_entry( InterpreterPIC& pic, int i, mapOop rMap,
                      oop delToSend, fint arg_count, int32 resSP );

 public:
  oop try_perform_prim( bool hasFailBlock,
                        bool& is_perform );

  oop  get_slot(slotDesc* sd);
  void set_slot(slotDesc* sd, oop x);

  
 protected:
 
  fint return_pc()  { return mi.length_codes - 1; }
  fint restart_pc() { return mi.length_codes;     }
  void local_slot_desc(interpreter*& interp, abstract_vframe*& vf, slotDesc*& sd);
  void block_scope_and_desc_of_home( frame*& block_scope_frame, int32& block_desc);


  
 public:
  bool is_return_patched() { return return_patch_reason != not_patched; }
  void patch_return(ReturnPatchReason x) { return_patch_reason = x; }
  ReturnPatchReason get_return_patch_reason() { 
    return return_patch_reason; }
    
  void print();

 protected:
  void setup_for_method( );
  void setup_for_block(  );
  void start_NLR(oop res);
  void continue_NLR();
  
private:
  // must be called after every bytecode
  void transfer_back_to_twains_process_if_stepping_or_stopping_pre();
};

extern void InterpreterLookup_cont( simpleLookup *L, int32 arg_count);


// for scavenging, see frame.c
// Don't do:
//    for (p = is->args;  p < &is->args[is->length_args];  p++) { template; }
// because when args are passed from interpreter, they are iterated
// in its stack anyway. This means that non-interp calling interp
// must arrange to scavenge, etc the top-level args.

# define INTERPRETER_ITERATOR(interp, template, zap, reinit) \
  { \
    ABSTRACT_INTERPRETER_ITERATOR(interp, template, zap, reinit) \
    \
    oop* p; \
    \
    p =       &(interp)->receiver;      template; \
    p = (oop*)&(interp)->selector;      template; \
    p = (oop*)&(interp)->delegatee;     template; \
    p = (oop*)&(interp)->method_object; template; \
    p =       &(interp)->self;          template; \
    p = (interp)->methodHolder_addr();  template; \
    \
    for ( p =  (interp)->locals;  \
          p < &(interp)->locals[(interp)->length_locals()]; \
        ++p) { \
      template; \
    } \
    for ( p = (interp)->cloned_blocks;  \
          p < &(interp)->cloned_blocks[(interp)->mi.length_literals]; \
        ++p) { \
      if (*p != NULL) { template; }  \
    } \
    for (p = (interp)->stack;  p < &(interp)->stack[(interp)->sp];  p++) { \
      template; \
    } \
    if (zap) for ( ; p < &(interp)->stack[(interp)->length_stack()]; p++) { \
      *p = badOop; \
    } \
    p =       &(interp)->rcvToSend;  template; \
    p = (oop*)&(interp)->selToSend;  template; \
  }
  
  
// new style:

class InterpreterIterator: public StackObj {
 private:
  interpreter* interp;
  OopClosure* oop_closure;
  bool zap;
  bool reinit;
  
 public:
  InterpreterIterator(interpreter* i, OopClosure* oc, bool z, bool r) {
    interp = i;  oop_closure = oc;  zap = z;  reinit = r;
    do_all();
  }
 private:
  void do_all() {
    ABSTRACT_INTERPRETER_ITERATOR(interp, oop_closure->do_oop(p), zap, reinit) 
    
    oop* p; 
    
    p =       &(interp)->receiver;             oop_closure->do_oop(p); 
    p = (oop*)&(interp)->selector;             oop_closure->do_oop(p); 
    p = (oop*)&(interp)->delegatee;            oop_closure->do_oop(p); 
    p = (oop*)&(interp)->method_object;        oop_closure->do_oop(p); 
    p =       &(interp)->self;                 oop_closure->do_oop(p); 
    p =        (interp)->methodHolder_addr();  oop_closure->do_oop(p);
    
    for ( p =  (interp)->locals;  
          p < &(interp)->locals[(interp)->length_locals()]; 
        ++p) { 
      oop_closure->do_oop(p); 
    }
    for ( p = (interp)->cloned_blocks;   
          p < &(interp)->cloned_blocks[(interp)->mi.length_literals];
        ++p) { 
      if (*p != NULL) { oop_closure->do_oop(p); } 
    }
    for (p = (interp)->stack;  p < &(interp)->stack[(interp)->sp];  p++) {
      oop_closure->do_oop(p);
    }
    if (zap) for ( ; p < &(interp)->stack[(interp)->length_stack()]; p++) { 
      *p = badOop;
    }
    p =       &(interp)->rcvToSend;  oop_closure->do_oop(p);
    p = (oop*)&(interp)->selToSend;  oop_closure->do_oop(p);

    // If a lookup is in progress on this interpreter activation, walk its
    // captured oops too so a scavenge / mark / etc. updates them in place.
    // See interpreter::lookup_in_progress for why.
    if ((interp)->lookup_in_progress)
      (interp)->lookup_in_progress->oops_do(oop_closure);
  }
};
    
