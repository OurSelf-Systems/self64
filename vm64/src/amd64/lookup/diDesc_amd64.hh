# if defined(__x86_64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


  /* x86-64 DI (dependency-info) site layout (cf. diDesc_aarch64.hh).

     As with sendDescs, the jump target is a plain 8-byte address word,
     reached with JMP [rip+0], so repatching is an atomic store with no
     icache concern.  The reference point (offset 0) is the address just
     past the JMP instruction -- which is also the word itself; the code
     generator must align it to 8 bytes.

     Format (byte offsets from the reference point):

     -6: jmp [rip+0]          ; FF 25 00 00 00 00
      0: target address       ; the patchable "jump address" (8 bytes)
      8: nmln next            ; dependency link (8 bytes)
     16: nmln prev            ; dependency link (8 bytes)
     */

 public:

  enum {
    di_jump_address_offset  =  0,  // an address word, not a branch
    di_depend_offset        =  8,

    di_addr_desc_offset     = di_jump_address_offset
  };
# endif // defined(__x86_64__)
