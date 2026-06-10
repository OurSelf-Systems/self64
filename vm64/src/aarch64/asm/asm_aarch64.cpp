# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "asm_aarch64.hh"

# include "_asm_aarch64.cpp.incl"

# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

Assembler* theAssembler;      // current assembler for instructions

Assembler::Assembler(int32 instsSize, int32 locsSize, bool pr, bool isInstrs)
  : BaseAssembler(instsSize, locsSize, pr, isInstrs) {
  _nlits = 0;
}


void Assembler::emit32(a64_inst w) {
  assert((offset() & 3) == 0, "instruction stream must stay 4-aligned");
  if (printing) lprintf("  .inst 0x%08x\n", (unsigned long)w);
  Alloc(a64_inst);
  *data = w;
}


void Assembler::Backpatch(pc_t destp, pc_t target) {
  a64_inst w = *(a64_inst*)destp;
  int rel_words = (int)((target - destp) >> 2);
  assert(((target - destp) & 3) == 0, "branch target must be 4-aligned");
  a64_inst patched = a64_patch_branch(w, rel_words);
  assert(patched != 0, "Backpatch: not a patchable instruction");
  *(a64_inst*)destp = patched;
  lastBackpatch = destp;
}


// ---- loads/stores ------------------------------------------------------

static inline bool fits_uoff8(fint off) { return off >= 0 && off <= 32760 && (off & 7) == 0; }
static inline bool fits_simm9(fint off) { return off >= -256 && off <= 255; }

void Assembler::ldr(Location rt, Location rn, fint byte_offset) {
  if      (fits_uoff8(byte_offset)) emit32(a64_ldr_uoff(rt, rn, (unsigned)(byte_offset >> 3)));
  else if (fits_simm9(byte_offset)) emit32(a64_ldur(rt, rn, (int)byte_offset));
  else fatal1("ldr offset out of range: %ld", (long)byte_offset);
}
void Assembler::str(Location rt, Location rn, fint byte_offset) {
  if      (fits_uoff8(byte_offset)) emit32(a64_str_uoff(rt, rn, (unsigned)(byte_offset >> 3)));
  else if (fits_simm9(byte_offset)) emit32(a64_stur(rt, rn, (int)byte_offset));
  else fatal1("str offset out of range: %ld", (long)byte_offset);
}
void Assembler::ldrb(Location rt, Location rn, fint byte_offset) {
  assert(byte_offset >= 0 && byte_offset <= 4095, "ldrb offset out of range");
  emit32(a64_ldrb_uoff(rt, rn, (unsigned)byte_offset));
}
void Assembler::strb(Location rt, Location rn, fint byte_offset) {
  assert(byte_offset >= 0 && byte_offset <= 4095, "strb offset out of range");
  emit32(a64_strb_uoff(rt, rn, (unsigned)byte_offset));
}
void Assembler::stp(Location rt1, Location rt2, Location rn, fint byte_offset) {
  assert((byte_offset & 7) == 0 && byte_offset >= -512 && byte_offset <= 504, "stp offset");
  emit32(a64_stp(rt1, rt2, rn, (int)(byte_offset >> 3)));
}
void Assembler::ldp(Location rt1, Location rt2, Location rn, fint byte_offset) {
  assert((byte_offset & 7) == 0 && byte_offset >= -512 && byte_offset <= 504, "ldp offset");
  emit32(a64_ldp(rt1, rt2, rn, (int)(byte_offset >> 3)));
}


// ---- constants ----------------------------------------------------------

void Assembler::mov_imm(Location rd, smi value) {
  unsigned long long v = (unsigned long long)value;
  // count halfwords that are 0000 vs ffff to pick movz- or movn-anchored form
  int zeros = 0, ones = 0;
  for (int hw = 0; hw < 4; hw++) {
    unsigned h = (unsigned)((v >> (hw * 16)) & 0xFFFF);
    if      (h == 0)      zeros++;
    else if (h == 0xFFFF) ones++;
  }
  bool useMovn = ones > zeros;
  unsigned skip = useMovn ? 0xFFFFu : 0u;
  bool first = true;
  for (int hw = 0; hw < 4; hw++) {
    unsigned h = (unsigned)((v >> (hw * 16)) & 0xFFFF);
    if (h == skip) continue;
    if (first) {
      emit32(useMovn ? a64_movn(rd, ~h & 0xFFFFu, hw) : a64_movz(rd, h, hw));
      first = false;
    } else {
      emit32(a64_movk(rd, h, hw));
    }
  }
  if (first)  // all halfwords were `skip`: v is 0 or -1
    emit32(useMovn ? a64_movn(rd, 0, 0) : a64_movz(rd, 0, 0));
}

void Assembler::loadLiteral(Location rd, smi value, OperandType t) {
  if (_nlits >= MaxLits) fatal("literal pool overflow; raise MaxLits");
  _lits[_nlits].value = value;
  _lits[_nlits].type  = t;
  _lits[_nlits].site  = instsEnd;
  _nlits++;
  emit32(a64_ldr_lit(rd, 0)); // patched by flushLiteralPool
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
      // VM address: no GC tracking needed; record primitive call slots so
      // verifiers can identify them
      if (_lits[i].type == PVMAddressOperand) doAddOffset(_lits[i].type, false);
      Alloc(smi);
      *data = _lits[i].value;
    }
  }
  _nlits = 0;
}


// ---- arithmetic/logical ---------------------------------------------------

void Assembler::mov(Location rd, Location rm) {
  if (rd == rm) return;
  if (rd == SP || rm == SP) emit32(a64_mov_sp(rd, rm));
  else                      emit32(a64_mov_reg(rd, rm));
}

void Assembler::add(Location rd, Location rn, fint imm) {
  if (imm >= 0) { assert(imm <= 4095, "add imm range");  emit32(a64_add_imm(rd, rn, (unsigned)imm)); }
  else          { assert(imm >= -4095, "add imm range"); emit32(a64_sub_imm(rd, rn, (unsigned)(-imm))); }
}
void Assembler::sub(Location rd, Location rn, fint imm) { add(rd, rn, -imm); }
void Assembler::add (Location rd, Location rn, Location rm) { emit32(a64_add_reg(rd, rn, rm)); }
void Assembler::sub (Location rd, Location rn, Location rm) { emit32(a64_sub_reg(rd, rn, rm)); }
void Assembler::adds(Location rd, Location rn, Location rm) { emit32(a64_add_reg(rd, rn, rm, 0, true)); }
void Assembler::subs(Location rd, Location rn, Location rm) { emit32(a64_sub_reg(rd, rn, rm, 0, true)); }
void Assembler::cmp (Location rn, fint imm) {
  assert(imm >= 0 && imm <= 4095, "cmp imm range");
  emit32(a64_cmp_imm(rn, (unsigned)imm));
}
void Assembler::cmp (Location rn, Location rm) { emit32(a64_cmp_reg(rn, rm)); }

void Assembler::andd(Location rd, Location rn, smi bitmask) {
  int n, immr, imms;
  if (!a64_bitmask_imm((unsigned long long)bitmask, &n, &immr, &imms))
    fatal1("unencodable bitmask immediate %#lx", (unsigned long)bitmask);
  emit32(a64_and_imm(rd, rn, n, immr, imms));
}
void Assembler::tst(Location rn, smi bitmask) {
  int n, immr, imms;
  if (!a64_bitmask_imm((unsigned long long)bitmask, &n, &immr, &imms))
    fatal1("unencodable bitmask immediate %#lx", (unsigned long)bitmask);
  emit32(a64_ands_imm(a64_xzr, rn, n, immr, imms));
}
void Assembler::orr (Location rd, Location rn, Location rm) { emit32(a64_orr_reg(rd, rn, rm)); }
void Assembler::eor (Location rd, Location rn, Location rm) { emit32(a64_eor_reg(rd, rn, rm)); }
void Assembler::lsl (Location rd, Location rn, fint sh) { emit32(a64_lsl_imm(rd, rn, (int)sh)); }
void Assembler::lsr (Location rd, Location rn, fint sh) { emit32(a64_lsr_imm(rd, rn, (int)sh)); }
void Assembler::asr (Location rd, Location rn, fint sh) { emit32(a64_asr_imm(rd, rn, (int)sh)); }
void Assembler::mul (Location rd, Location rn, Location rm) { emit32(a64_mul(rd, rn, rm)); }
void Assembler::sdiv(Location rd, Location rn, Location rm) { emit32(a64_sdiv(rd, rn, rm)); }


// ---- branches --------------------------------------------------------------

// shared forward-reference protocol (cf. i386 word_branch_target):
// if L is undefined, emit offset 0 and register this site for Backpatch.
# define EMIT_BRANCH(L, ENCODE_WITH_REL)                                      \
  do {                                                                        \
    pc_t site = instsEnd;                                                     \
    int rel = 0;                                                              \
    if ((L) != NULL && (L)->isDefined())                                      \
      rel = (int)(((L)->target() - site) >> 2);                               \
    else if ((L) != NULL)                                                     \
      (L)->unify(new Label(printing, site));                                  \
    emit32(ENCODE_WITH_REL);                                                  \
  } while (0)

void Assembler::b(Label* L)                  { EMIT_BRANCH(L, a64_b(rel)); }
void Assembler::b(a64_cond cond, Label* L)   { EMIT_BRANCH(L, a64_bcond(cond, rel)); }
void Assembler::cbz (Location rt, Label* L)  { EMIT_BRANCH(L, a64_cbz(rt, rel)); }
void Assembler::cbnz(Location rt, Label* L)  { EMIT_BRANCH(L, a64_cbnz(rt, rel)); }
void Assembler::bl  (Label* L)               { EMIT_BRANCH(L, a64_bl(rel)); }

# undef EMIT_BRANCH

void Assembler::br (Location rn) { emit32(a64_br(rn)); }
void Assembler::blr(Location rn) { emit32(a64_blr(rn)); }
void Assembler::ret()            { emit32(a64_ret()); }
void Assembler::brk(fint imm16)  { emit32(a64_brk((unsigned)imm16)); }
void Assembler::nop()            { emit32(a64_nop()); }

void Assembler::align(fint bytes) {
  assert((bytes & 3) == 0, "alignment must be a multiple of the instruction size");
  while (offset() % bytes) nop();
}


// Type-test counting instrumentation: not implemented on aarch64
// (i386 didn't implement it either; only used under SICCountTypeTests).
void Assembler::startTypeTest(fint ncases, bool prologueCheck, bool immedOnly) {
  Unused(ncases); Unused(prologueCheck); Unused(immedOnly);
}
void Assembler::doOneTypeTest() {}
void Assembler::endTypeTest() {}


// print_code: annotated code disassembly, not yet implemented
void print_code(nmethod* nm, pc_t start, pc_t end) {
  Unused(nm); Unused(start); Unused(end);
  lprintf("[print_code: aarch64 disassembly not yet implemented]\n");
}

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == AARCH64_ARCH
