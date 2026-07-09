# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "asm_amd64.hh"

# include "_asm_amd64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

Assembler* theAssembler;      // current assembler for instructions

Assembler::Assembler(int32 instsSize, int32 locsSize, bool pr, bool isInstrs)
  : BaseAssembler(instsSize, locsSize, pr, isInstrs) {
  _nlits = 0;
}


void Assembler::emit(const x64_insn& w) {
  if (printing) {
    char buf[3 * sizeof(w.b) + 1];
    char* p = buf;
    for (fint k = 0; k < w.len; k++)
      p += sprintf(p, "%s0x%02x", k ? "," : "", w.b[k]);
    lprintf("  .byte %s\n", buf);
  }
  for (fint k = 0; k < w.len; k++) {
    Alloc(unsigned char);
    *data = w.b[k];
  }
}


void Assembler::Backpatch(pc_t destp, pc_t target) {
  unsigned char* end   = NULL;
  unsigned char* field = x64_patch_field((unsigned char*)destp, &end);
  assert(field != NULL, "Backpatch: not a patchable instruction");
  *(int32*)field = (int32)(target - (pc_t)end);
  lastBackpatch = destp;
}


// ---- loads/stores ------------------------------------------------------

static inline int disp32(fint disp) {
  assert(disp == (fint)(int32)disp, "displacement exceeds disp32");
  return (int)disp;
}

void Assembler::load(Location rt, Location base, fint disp) {
  emit(x64_mov_r_m(rt, base, disp32(disp)));
}
void Assembler::store(Location base, fint disp, Location rt) {
  emit(x64_mov_m_r(base, disp32(disp), rt));
}
void Assembler::load32(Location rt, Location base, fint disp) {
  emit(x64_mov32_r_m(rt, base, disp32(disp)));
}
void Assembler::store32(Location base, fint disp, Location rt) {
  emit(x64_mov32_m_r(base, disp32(disp), rt));
}
void Assembler::loadByte(Location rt, Location base, fint disp) {
  emit(x64_movzxb_r_m(rt, base, disp32(disp)));
}
void Assembler::storeByte(Location base, fint disp, Location rt) {
  emit(x64_mov_m_r8(base, disp32(disp), rt));
}
void Assembler::store_zero(Location base, fint disp) {
  emit(x64_mov_m_imm32(base, disp32(disp), 0));
}
void Assembler::store_zero32(Location base, fint disp) {
  emit(x64_mov32_m_imm32(base, disp32(disp), 0));
}
void Assembler::storeByte_zero(Location base, fint disp) {
  emit(x64_mov_m_imm8(base, disp32(disp), 0));
}
void Assembler::add32_mem(Location base, fint disp, fint imm) {
  emit(x64_add32_m_imm(base, disp32(disp), disp32(imm)));
}
void Assembler::lea(Location rt, Location base, fint disp) {
  emit(x64_lea_r_m(rt, base, disp32(disp)));
}


// ---- constants ----------------------------------------------------------

void Assembler::mov_imm(Location rd, smi value) {
  if (value == (smi)(int32)value)
    emit(x64_mov_r_imm32s(rd, (int)value));                 // sign-extends
  else if ((unsigned long long)value <= 0xFFFFFFFFull)
    emit(x64_mov32_r_imm32(rd, (int)value));                // zero-extends
  else
    emit(x64_mov_r_imm64(rd, (unsigned long long)value));
}

void Assembler::loadLiteral(Location rd, smi value, OperandType t) {
  if (_nlits >= MaxLits) fatal("literal pool overflow; raise MaxLits");
  _lits[_nlits].value = value;
  _lits[_nlits].type  = t;
  _lits[_nlits].site  = instsEnd;
  _nlits++;
  emit(x64_mov_r_rip(rd, 0)); // patched by flushLiteralPool
}

void Assembler::loadOopLiteral(Location rd, oop p) {
  loadLiteral(rd, smi(p), OopOperand);
}

void Assembler::loadAddressLiteral(Location rd, void* a, OperandType t) {
  loadLiteral(rd, smi(a), t);
}

void Assembler::flushLiteralPool() {
  if (_nlits == 0) return;
  // 8-align the pool so each word is a naturally aligned, atomically
  // updatable oop/address slot
  align(8);
  for (fint i = 0; i < _nlits; i++) {
    Backpatch(_lits[i].site, instsEnd);
    if (_lits[i].type == OopOperand) {
      Data(oop(_lits[i].value));      // records oop addrDesc for mem oops
    } else {
      // plain VMAddressOperand words need no loc; primitive, code-address
      // and backpatchable-send words are recorded so the PIC/sendDesc
      // machinery can find and repatch them
      if (_lits[i].type != VMAddressOperand) doAddOffset(_lits[i].type, false);
      Alloc(smi);
      *data = _lits[i].value;
    }
  }
  _nlits = 0;
}


void Assembler::DataPtr(smi v) {
  if (printing) lprintf("  .quad %#lx\n", (unsigned long)v);
  Alloc(smi);
  *data = v;
}


// ---- arithmetic/logical ---------------------------------------------------

void Assembler::mov(Location rd, Location rm) {
  if (rd == rm) return;
  emit(x64_mov_r_r(rd, rm));
}

void Assembler::add(Location rd, Location rm) { emit(x64_add_r_r(rd, rm)); }
void Assembler::add(Location rd, fint imm)    { emit(x64_add_r_imm(rd, disp32(imm))); }
void Assembler::sub(Location rd, Location rm) { emit(x64_sub_r_r(rd, rm)); }
void Assembler::sub(Location rd, fint imm)    { emit(x64_sub_r_imm(rd, disp32(imm))); }
void Assembler::cmp(Location rn, fint imm)    { emit(x64_cmp_r_imm(rn, disp32(imm))); }
void Assembler::cmp(Location rn, Location rm) { emit(x64_cmp_r_r(rn, rm)); }

void Assembler::andd(Location rd, smi bitmask) {
  assert(bitmask == (smi)(int32)bitmask, "bitmask exceeds imm32 (sign-extended)");
  emit(x64_and_r_imm(rd, (int)bitmask));
}
void Assembler::andd(Location rd, Location rm) { emit(x64_and_r_r(rd, rm)); }
void Assembler::orr (Location rd, Location rm) { emit(x64_or_r_r(rd, rm)); }
void Assembler::xorr(Location rd, Location rm) { emit(x64_xor_r_r(rd, rm)); }
void Assembler::tst(Location rn, smi bitmask) {
  assert(bitmask == (smi)(int32)bitmask, "bitmask exceeds imm32 (sign-extended)");
  emit(x64_test_r_imm(rn, (int)bitmask));
}
void Assembler::tst(Location rn, Location rm) { emit(x64_test_r_r(rn, rm)); }
void Assembler::neg (Location rd)              { emit(x64_neg_r(rd)); }

void Assembler::shl(Location rd, fint sh) { emit(x64_shl_r_imm(rd, (int)sh)); }
void Assembler::shr(Location rd, fint sh) { emit(x64_shr_r_imm(rd, (int)sh)); }
void Assembler::sar(Location rd, fint sh) { emit(x64_sar_r_imm(rd, (int)sh)); }
void Assembler::shl_cl(Location rd)       { emit(x64_shl_r_cl(rd)); }
void Assembler::shr_cl(Location rd)       { emit(x64_shr_r_cl(rd)); }
void Assembler::sar_cl(Location rd)       { emit(x64_sar_r_cl(rd)); }

void Assembler::imul(Location rd, Location rm) { emit(x64_imul_r_r(rd, rm)); }
void Assembler::cqo ()                         { emit(x64_cqo()); }
void Assembler::idiv(Location rm)              { emit(x64_idiv_r(rm)); }


// ---- branches --------------------------------------------------------------

// shared forward-reference protocol (cf. asm_aarch64.cpp EMIT_BRANCH):
// if L is undefined, emit offset 0 and register this site for Backpatch.
// LEN is the encoded instruction length; rel32 counts from the insn end.
# define EMIT_BRANCH(L, LEN, ENCODE_WITH_REL)                                 \
  do {                                                                        \
    pc_t site = instsEnd;                                                     \
    int rel = 0;                                                              \
    if ((L) != NULL && (L)->isDefined())                                      \
      rel = (int)((L)->target() - (site + (LEN)));                            \
    else if ((L) != NULL)                                                     \
      (L)->unify(new Label(printing, site));                                  \
    emit(ENCODE_WITH_REL);                                                    \
  } while (0)

void Assembler::jmp (Label* L)              { EMIT_BRANCH(L, 5, x64_jmp_rel32(rel)); }
void Assembler::jcc (x64_cond cc, Label* L) { EMIT_BRANCH(L, 6, x64_jcc_rel32(cc, rel)); }
void Assembler::call(Label* L)              { EMIT_BRANCH(L, 5, x64_call_rel32(rel)); }

# undef EMIT_BRANCH

void Assembler::call_reg(Location rn) { emit(x64_call_r(rn)); }
void Assembler::jmp_reg (Location rn) { emit(x64_jmp_r(rn)); }

void Assembler::call_mem(pc_t dataWord) {
  emit(x64_call_rip(disp32(dataWord - (instsEnd + 6))));
}
void Assembler::jmp_mem(pc_t dataWord) {
  emit(x64_jmp_rip(disp32(dataWord - (instsEnd + 6))));
}

void Assembler::ret  ()            { emit(x64_ret()); }
void Assembler::leave()            { emit(x64_leave()); }
void Assembler::push (Location rn) { emit(x64_push_r(rn)); }
void Assembler::pop  (Location rn) { emit(x64_pop_r(rn)); }
void Assembler::int3 ()            { emit(x64_int3()); }
void Assembler::ud2  ()            { emit(x64_ud2()); }
void Assembler::nop  ()            { emit(x64_nop()); }

void Assembler::align(fint bytes) {
  while (offset() % bytes) nop();
}

void Assembler::align_end(fint bytes, fint len) {
  while ((offset() + len) % bytes) nop();
}


// Type-test counting instrumentation: not implemented on amd64
// (neither i386 nor aarch64 implement it; only used under SICCountTypeTests).
void Assembler::startTypeTest(fint ncases, bool prologueCheck, bool immedOnly) {
  Unused(ncases); Unused(prologueCheck); Unused(immedOnly);
}
void Assembler::doOneTypeTest() {}
void Assembler::endTypeTest() {}


// print_code: annotated code disassembly, not yet implemented
void print_code(nmethod* nm, pc_t start, pc_t end) {
  Unused(nm); Unused(start); Unused(end);
  lprintf("[print_code: amd64 disassembly not yet implemented]\n");
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == X86_64_ARCH
