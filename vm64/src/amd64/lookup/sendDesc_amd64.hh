# if defined(__x86_64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

/* x86-64 send site layout (cf. sendDesc_aarch64.hh).

   Unlike i386, the call target is not a patched branch displacement: it is
   a plain 8-byte address word inside the sendDesc data block, called
   through with CALL [rip+16].  Rebinding a send is therefore a single
   aligned 8-byte store -- atomic on x86-64, no branch-range limit, and the
   x86 instruction cache is coherent so there is nothing to flush.

   The code generator must align send sites so the return PC (offset 0) is
   8-byte aligned (align_end(8, 6) before the call); oop and pointer fields
   below are then naturally aligned, and a sendDesc* is indistinguishable
   from an untagged pointer (Tag_Mask bits clear), which the lookup code
   relies on.

   Field offsets from +16 on match sendDesc_aarch64.hh exactly, so the
   descriptor/PIC machinery ports unchanged.

   Format (byte offsets from the return PC):

   -6: call [rip+16]          ; FF 15; rip = return PC, so +16 is the word
    0: jmp  past_desc         ; E9 rel32; branch around the data block
    5: jmp  NLR_code          ; non-local return entry point (E9 rel32)
   10: used-register mask     ; RegisterString, 4 bytes
   14: (padding, 2 bytes)
   16: target address         ; the inline-cache "jump address" (8 bytes)
   24: nmln next              ; dependency link (8 bytes)
   32: nmln prev              ; dependency link (8 bytes)
   40: selector / arg count   ; mutually exclusive (8 bytes)
   48: lookupType             ; 8-byte slot, low word used
   56: [delegatee]            ; optional (8 bytes)
   */

 public:
  enum {
    call_instruction_offset           = -6,  // the call [rip+16]
    branch_around_desc_offset         =  0,
    non_local_return_offset           =  5,  // one jmp rel32
    mask_offset                       = 10,  // must be here for primitives
    jump_address_offset               = 16,  // address word, not a branch
    depend_offset                     = 24,  // two words of nmln
    selector_offset                   = 40,  // ** these are
    arg_count_offset                  = 40,  // ** mutually exclusive
    lookupType_offset                 = 48,
    delegatee_offset                  = 56,

    normal_sendDesc_end_offset        = delegatee_offset,

    // since always have branch, send getPrimCallEndOffset to the branch
       abortable_prim_continue_offset = branch_around_desc_offset,
    nonabortable_prim_continue_offset = branch_around_desc_offset
  };

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // defined(__x86_64__)
