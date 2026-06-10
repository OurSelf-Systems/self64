# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


  /* aarch64 DI (dependency-info) site layout.

     As with sendDescs (see sendDesc_aarch64.hh), the jump target is a plain
     8-byte address word loaded with a pc-relative LDR, so repatching is an
     atomic store with no icache flush.  The reference point (offset 0) is
     the address just past the BR instruction; the code generator must align
     it to 8 bytes.

     Format (byte offsets from the reference point):

     -8: ldr x16, +0          ; load target address word below
     -4: br  x16
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
# endif // defined(__aarch64__)
