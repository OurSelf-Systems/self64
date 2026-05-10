/* Sun-$Revision: 30.9 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif



inline void abstract_interpreter_bytecode_info::decode(fint c) {
  code= c;
  op= getOp(code);
  x= getIndex(code);
}


inline fint abstract_interpreter::get_argument_count() {
  # if GENERATE_DEBUGGING_AIDS
  if ( CheckAssertions   &&   mi.instruction_set == TWENTIETH_CENTURY_PLUS_ARGUMENT_COUNT_INSTRUCTION_SET
  &&   is.argument_count != get_selector()->arg_count()) {
    stringOop sel = get_selector();
#   if DIAG_ACTIVATION_DUMP /* DIAGNOSTIC: dump targeted info before fatal so we can tell whether the live selector literal got swapped after argument_count was set. References is.saved_selector_*, is.pre_send_*, pc_ring — all gated on the same toggle.  -- claude & dmu May 2026 */
    lprintf("\n--- argument_count fatal targeted dump ---\n");
    lprintf("is.argument_count       = %d\n", is.argument_count);
    lprintf("is.index                = %d\n", is.index);
    lprintf("live selector oop       = %p\n", (void*)sel);
    lprintf("live selector arg_count = %d\n", sel->arg_count());
    lprintf("live selector len       = %d\n", sel->length());
    lprintf("live selector bytes     = '");
    for (fint i = 0; i < sel->length() && i < 64; ++i) lprintf("%c", sel->bytes()[i]);
    lprintf("'\nsaved_selector_oop      = %p\n", (void*)is.saved_selector_oop);
    lprintf("saved_selector_len      = %d\n", is.saved_selector_len);
    lprintf("saved_selector_str      = '%s'\n", is.saved_selector_str);
    lprintf("oops match: %s   strings match: %s\n",
            (void*)sel == (void*)is.saved_selector_oop ? "YES" : "NO",
            (is.saved_selector_len == sel->length()
             && memcmp(is.saved_selector_str, sel->bytes(),
                       sel->length() < (fint)sizeof(is.saved_selector_str)-1
                         ? sel->length() : sizeof(is.saved_selector_str)-1) == 0)
              ? "YES" : "NO");
    lprintf("--- captured at send entry (pre_*_SEND_CODE) ---\n");
    lprintf("pre_send_was_implicit         = %s\n",
            is.pre_send_was_implicit ? "true" : "false");
    lprintf("pre_send_pc                   = %d\n", is.pre_send_pc);
    lprintf("pre_send_argument_count_state = %d\n",
            is.pre_send_argument_count_state);
    lprintf("pre_send_index_state          = %d\n",
            is.pre_send_index_state);
    lprintf("pre_send_selector_oop         = %p\n",
            (void*)is.pre_send_selector_oop);
    lprintf("pre_send_selector_arg_count   = %d\n",
            is.pre_send_selector_arg_count);
    lprintf("pre_send_selector_len         = %d\n",
            is.pre_send_selector_len);
    lprintf("pre_send_selector_str         = '%s'\n",
            is.pre_send_selector_str);
    lprintf("pre_send -> live oop match:    %s\n",
            (void*)is.pre_send_selector_oop == (void*)sel ? "YES" : "NO");
    print_activation_diag("get_argument_count mismatch");
    lprintf("--- offending method (legacy fields) ---\n");
    lprintf("mi.instruction_set    = %d %s\n",
            (int)mi.instruction_set,
            mi.instruction_set == TWENTIETH_CENTURY_PLUS_ARGUMENT_COUNT_INSTRUCTION_SET
              ? "(20C+ARGCOUNT, ARGUMENT_COUNT mandatory before n>0 sends)"
              : mi.instruction_set == TWENTIETH_CENTURY_INSTRUCTION_SET
                ? "(20C, arg_count derived from selector)"
                : "(unknown)");
    lprintf("mi.length_codes       = %d\n", (int)mi.length_codes);
    lprintf("mi.length_literals    = %d\n", (int)mi.length_literals);
    lprintf("mi._map_oop (method)  = %p\n", (void*)mi.map_oop());
    {
      oop invsel = diag_invocation_selector();
      lprintf("invocation selector   = %p", (void*)invsel);
      if (invsel != NULL && invsel->is_mem() && invsel->is_string()) {
        stringOop ss = stringOop(invsel);
        lprintf(" '");
        for (fint i = 0; i < ss->length() && i < 64; ++i)
          lprintf("%c", ss->bytes()[i]);
        lprintf("' arg_count=%d", (int)ss->arg_count());
      }
      lprintf("\n");
    }
    // DIAGNOSTIC: dump per-activation pc ring buffer  -- claude & dmu May 2026
    lprintf("dispatched-pc ring (oldest -> newest, %d total):\n",
            pc_ring_total);
    {
      int n = pc_ring_total < PC_RING_SIZE ? pc_ring_total : PC_RING_SIZE;
      int start = pc_ring_total < PC_RING_SIZE ? 0
                                               : pc_ring_total % PC_RING_SIZE;
      lprintf("  ");
      for (int k = 0; k < n; ++k) {
        fint p = pc_ring[(start + k) % PC_RING_SIZE];
        u_char c = (p >= 0 && p < mi.length_codes) ? mi.codes[p] : 0;
        lprintf("%d(op=%d,x=%d) ", (int)p,
                p >= 0 ? (int)getOp(c) : -1,
                p >= 0 ? (int)getIndex(c) : -1);
      }
      lprintf("\n");
    }
    lprintf("bytecode window around pc=%d (op,x):\n", (int)pc);
    {
      fint lo = pc > 6 ? pc - 6 : 0;
      fint hi = pc + 3 < mi.length_codes ? pc + 3 : mi.length_codes - 1;
      for (fint i = lo; i <= hi; ++i) {
        u_char c = mi.codes[i];
        lprintf("  codes[%d]%s = 0x%02x  op=%d x=%d\n",
                (int)i, i == pc ? " <-- pc" : "",
                (unsigned)c, (int)getOp(c), (int)getIndex(c));
      }
    }
    lprintf("------------------------------------------\n");
#   endif
    fatal2("argument_count %d does not match selector's argument count %d",
           is.argument_count,
           sel->arg_count());
  }
  # endif
  return mi.instruction_set == TWENTIETH_CENTURY_PLUS_ARGUMENT_COUNT_INSTRUCTION_SET ? is.argument_count : get_selector()->arg_count();
}


inline abstract_interpreter_interbytecode_state::abstract_interpreter_interbytecode_state() {
  reset_lexical_level();
  reset_index();
  reset_send_modifiers();
  last_literal= NULL;
# if DIAG_ACTIVATION_DUMP /* DIAGNOSTIC: zero-init the saved_selector_* and pre_send_* capture fields.  -- claude & dmu May 2026 */
  saved_selector_oop= NULL;
  saved_selector_str[0]= '\0';
  saved_selector_len= 0;
  pre_send_selector_oop= NULL;
  pre_send_selector_arg_count= -1;
  pre_send_selector_len= -1;
  pre_send_selector_str[0]= '\0';
  pre_send_argument_count_state= 0;
  pre_send_index_state= 0;
  pre_send_pc= -1;
  pre_send_was_implicit= false;
# endif
}
  

inline abstract_interpreter::abstract_interpreter(oop meth)
 : mi((methodMap*)meth->map()) {
  pc= mi.firstBCI();
  error_msg= NULL;
# if DIAG_ACTIVATION_DUMP /* DIAGNOSTIC: per-activation pc ring buffer init  -- claude & dmu May 2026 */
  pc_ring_total= 0;
  for (int i = 0; i < PC_RING_SIZE; ++i) pc_ring[i] = -1;
# endif
}


inline abstract_interpreter::abstract_interpreter(methodMap *m)
 : mi(m) {
  pc= mi.firstBCI();
  error_msg= NULL;
# if DIAG_ACTIVATION_DUMP /* DIAGNOSTIC: per-activation pc ring buffer init  -- claude & dmu May 2026 */
  pc_ring_total= 0;
  for (int i = 0; i < PC_RING_SIZE; ++i) pc_ring[i] = -1;
# endif
}


inline abstract_interpreter::abstract_interpreter(byteVectorOop codes,
                                                  objVectorOop literals)
 : mi(codes, literals) {
  pc= mi.firstBCI();
  error_msg= NULL;
# if DIAG_ACTIVATION_DUMP /* DIAGNOSTIC: per-activation pc ring buffer init  -- claude & dmu May 2026 */
  pc_ring_total= 0;
  for (int i = 0; i < PC_RING_SIZE; ++i) pc_ring[i] = -1;
# endif
}


inline stringOop abstract_interpreter::get_selector() { 
  oop s = get_literal();
  return check(check_selector_string, s)  
    ?  stringOop(s)  
    :  new_string("Error: cannot find selector");
}
