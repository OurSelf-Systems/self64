# if  TARGET_ARCH == I386_ARCH || TARGET_ARCH == X86_64_ARCH || TARGET_ARCH == AARCH64_ARCH
/* Sun-$Revision: 1.3 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "stubs_amd64.hh"
# include "_stubs_amd64.cpp.incl"

# pragma warn_unusedarg off


// asm routines (only needed for SIC:)



extern "C" {
  oop UncommonBranch(...) { fatal("unimp intel");  return NULL; }
  // Note: do not have a stack frame yet; link is link of caller
  // Also, they must preserve Temp1 and Temp2
  }

// Only needed for interpreter/compiler interoperation:
extern "C" {  oop ReturnResult_stub(...) { fatal("unimp intel");  return NULL; } }
oop ReturnResult_stub_result;


# if TARGET_ARCH == AARCH64_ARCH && defined(SIC_COMPILER)

// EnterSelf: the C++ -> compiled-Self entry point.
//
// Generated into the stub zone at first use rather than written as static
// assembly: its inline sendDesc carries nmln words the runtime writes, and
// macOS text pages are immutable -- the JIT zone is the one writable+
// executable home.  Generating it also lets C++ supply the LookupType
// constant and lays the data block out via the same sendDesc offsets the
// compiler uses.
//
// The stub: builds a C frame record, saves the callee-saved registers
// compiled Self code may trash, reserves a Self outgoing area (lr hole at
// [sp+0], receiver at [sp+1], one argument at [sp+2]), and calls the
// nmethod.  Its return PC is the bottom-of-Self-stack sentinel
// (firstSelfFrame_returnPC) that frame walking terminates on, followed by
// the firstSelfFrame sendDesc.  An NLR arriving here behaves like a
// normal return (the result is already in x0).

oop (*EnterSelf_generated)(oop recv, char* entryPoint, oop arg1) = NULL;
static char* SendMessage_stub_generated = NULL;
void generate_EnterSelf();   // below

// the zone-resident lookup stub that empty/missed inline caches call;
// generated alongside EnterSelf
char* aarch64_SendMessage_stub() {
  if (SendMessage_stub_generated == NULL) generate_EnterSelf();
  return SendMessage_stub_generated;
}

// SendMessage_stub: reached through a send site's target word on an
// inline-cache miss.  On entry lr is the send's return PC, which IS the
// sendDesc; the receiver and arguments sit in the sender's outgoing area
// ([sp+1] up), and fp is the sender's frame.  Call the C lookup (which
// compiles and rebinds the cache), then tail-jump to the returned entry
// point with lr restored so the callee's prologue saves the right PC.
static void generate_SendMessage_stub(Assembler* a) {
  a->Comment("SendMessage_stub");
  a->sub(SP, SP, 16);
  a->stp(fp, lr, SP, 0);          // scratch frame record (fp unchanged)
  a->mov(x0, lr);                 // ic = the sendDesc
  a->mov(x1, fp);                 // lookupFrame = the sending frame
  a->ldr(x2, SP, 16 + 1*oopSize); // receiver from sender's outgoing area
  a->mov_imm(x3, 0);              // perform_selector  (normal sends)
  a->mov_imm(x4, 0);              // perform_delegatee
  a->mov_imm(x5, 0);              // arg1
  a->loadAddressLiteral(x16, (void*)&SendMessage, VMAddressOperand);
  a->blr(x16);
  a->mov(x17, x0);                // target entry point
  a->ldp(fp, lr, SP, 0);
  a->add(SP, SP, 16);
  a->br(x17);
}

void generate_EnterSelf() {
  ResourceMark rm;
  Assembler* saved = theAssembler;
  Assembler* a = theAssembler = new Assembler(1024, 64, false, true);

  // C prologue: frame record, then save x19..x28
  a->sub(SP, SP, 16);
  a->stp(fp, lr, SP, 0);
  a->mov(fp, SP);
  a->sub(SP, SP, 80);
  for (fint i = 0; i < 5; i++)
    a->stp(Location(x19 + 2*i), Location(x20 + 2*i), SP, 16*i);

  // Self outgoing area: [sp+0] lr hole, [sp+1] receiver, [sp+2] arg, pad
  a->sub(SP, SP, 32);
  a->str(x0, SP, 1 * oopSize);
  a->str(x2, SP, 2 * oopSize);

  // the return PC after blr must be 8-aligned (it is the sendDesc address)
  if (((a->offset() + 4) & 7) != 0) a->nop();
  a->blr(x1);

  fint retPC_offset = a->offset();
  Label done(a->printing);
  a->b(&done);                       // @0  branch around desc; normal return
  a->Data((int32)0, false);          // @4  mask
  a->b(&done);                       // @8  NLR entry: result already in x0
  a->Data((int32)0, false);          // @12 pad
  a->DataPtr(0);                     // @16 jump_address (never rebound)
  a->DataPtr(0);                     // @24 nmln next
  a->DataPtr(0);                     // @32 nmln prev
  a->DataPtr(0);                     // @40 selector (static lookup: unused)
  a->DataPtr(smi(StaticNormalLookupType));  // @48 lookupType
  fint descEnd_offset = a->offset();
  done.define();

  // epilogue: drop outgoing area, restore registers, return (result in x0)
  a->add(SP, SP, 32);
  for (fint i = 0; i < 5; i++)
    a->ldp(Location(x19 + 2*i), Location(x20 + 2*i), SP, 16*i);
  a->add(SP, SP, 80);
  a->ldp(fp, lr, SP, 0);
  a->add(SP, SP, 16);
  a->ret();

  a->align(8);
  fint sendMessage_offset = a->offset();
  generate_SendMessage_stub(a);
  a->flushLiteralPool();   // &SendMessage literal; ldr-lit is pc-relative

  // copy into the stub zone and publish (the allocator itself writes
  // free-list metadata inside the zone, so it needs write mode too)
  int32 len = a->instsLen();
  char* dst;
  {
    JITWriteScope ws;
    dst = (char*)Memory->code->stubs->allocate(len);
    if (dst == NULL) fatal("no stub-zone space for EnterSelf");
    copy_bytes(a->instsStart, dst, len);
  }
  MachineCache::flush_instruction_cache_range(dst, dst + len);

  firstSelfFrame_returnPC   = dst + retPC_offset;
  firstSelfFrameSendDescEnd = dst + descEnd_offset;
  EnterSelf_generated = (oop (*)(oop, char*, oop))dst;
  SendMessage_stub_generated = dst + sendMessage_offset;

  // self-check: the generated block must parse as the first sendDesc
  sendDesc* f = sendDesc::sendDesc_from_return_PC(firstSelfFrame_returnPC);
  assert(f->raw_lookupType() == StaticNormalLookupType,
         "generated firstSelfFrame sendDesc has wrong lookup type");
  assert((char*)f + f->endOffset() == firstSelfFrameSendDescEnd,
         "generated firstSelfFrame sendDesc has wrong size");

  theAssembler = saved;
}

# endif // TARGET_ARCH == AARCH64_ARCH && defined(SIC_COMPILER)

# endif // TARGET_ARCH == I386_ARCH || TARGET_ARCH == X86_64_ARCH || TARGET_ARCH == AARCH64_ARCH
