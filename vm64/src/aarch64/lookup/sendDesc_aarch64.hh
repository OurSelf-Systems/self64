# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

/* aarch64 send site layout.

   Unlike i386, the call target is not a patched branch displacement: it is
   a plain 8-byte address word inside the sendDesc data block, loaded with a
   pc-relative LDR and called with BLR.  Rebinding a send is therefore a
   single aligned 8-byte store -- atomic on aarch64, no branch-range limit,
   and no instruction-cache flush.

   The code generator must align send sites so the return PC (offset 0) is
   8-byte aligned; oop and pointer fields below are then naturally aligned,
   and a sendDesc* is indistinguishable from an untagged pointer (Tag_Mask
   bits clear), which the lookup code relies on.

   Format (byte offsets from the return PC):

   -8: ldr  x16, +16          ; load target address word below
   -4: blr  x16               ; the call; return PC points just past it
    0: b    past_desc         ; branch around the data block
    4: used-register mask     ; RegisterString, 4 bytes
    8: b    NLR_code          ; non-local return entry point
   12: (padding, 4 bytes)
   16: target address         ; the inline-cache "jump address" (8 bytes)
   24: nmln next              ; dependency link (8 bytes)
   32: nmln prev              ; dependency link (8 bytes)
   40: selector / arg count   ; mutually exclusive (8 bytes)
   48: lookupType             ; 8-byte slot, low word used
   56: [delegatee]            ; optional (8 bytes)
   */

 public:
  enum {
    call_instruction_offset           = -8,  // start of ldr/blr pair
    branch_around_desc_offset         =  0,
    mask_offset                       =  4,  // must be here for primitives
    non_local_return_offset           =  8,  // one instruction
    jump_address_offset               = 16,  // address word, not a branch
    depend_offset                     = 24,  // two words of nmln
    selector_offset                   = 40,  // ** these are
    arg_count_offset                  = 40,  // ** mutually exclusive
    lookupType_offset                 = 48,
    delegatee_offset                  = 56,

    normal_sendDesc_end_offset        = delegatee_offset,

    // words from a lookup stub's frame record to the sender's outgoing
    // receiver slot: the {fp, lr} record plus the caller's lr hole
    lookup_stub_record_to_rcvr_words  = 3,

    // since always have branch, send getPrimCallEndOffset to the branch
       abortable_prim_continue_offset = branch_around_desc_offset,
    nonabortable_prim_continue_offset = branch_around_desc_offset
  };

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // defined(__aarch64__)
