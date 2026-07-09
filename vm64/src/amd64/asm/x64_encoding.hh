/* Copyright 2026 AUTHORS.
   See the LICENSE file for license information. */

// Pure x86-64 instruction encoding helpers.
//
// Deliberately free of VM dependencies (plain unsigned types, no oops, no
// Locations) so the encodings can be verified standalone against a real
// assembler -- see vm64/tools/test_x64_encoding.sh, which assembles the
// same forms with the system assembler and compares bytes.  The sibling
// aarch64 header is a64_encoding.hh.
//
// Register numbers are hardware numbers 0..15 (rax=0, rcx=1, rdx=2, rbx=3,
// rsp=4, rbp=5, rsi=6, rdi=7, r8..r15 = 8..15); bit 3 goes into the REX
// prefix.  All forms are 64-bit (REX.W) unless the name says otherwise.
//
// Unlike A64's fixed 32-bit words, an x86 instruction is a variable-length
// byte string, so every encoder returns an x64_insn value (length + bytes).
//
// Branches are always emitted in their rel32 forms (never rel8), so a
// branch site has a fixed shape and Label backpatching can rewrite the
// offset in place -- the x86 analog of A64's fixed-width patchable words.

# ifndef X64_ENCODING_HH
# define X64_ENCODING_HH

struct x64_insn {              // one encoded instruction
  unsigned char len;
  unsigned char b[15];
};

typedef unsigned char x64_byte;

// hardware register numbers (only for readability inside encoders/tests;
// the VM's Location enum in regs_amd64.hh mirrors these values)
enum {
  x64_rax = 0, x64_rcx = 1, x64_rdx = 2, x64_rbx = 3,
  x64_rsp = 4, x64_rbp = 5, x64_rsi = 6, x64_rdi = 7,
  x64_r8  = 8, x64_r9  = 9, x64_r10 = 10, x64_r11 = 11,
  x64_r12 = 12, x64_r13 = 13, x64_r14 = 14, x64_r15 = 15
};

// ---------------------------------------------------------------- primitives

inline void x64_put(x64_insn* i, unsigned byte) {
  i->b[i->len++] = (x64_byte)byte;
}
inline void x64_put32(x64_insn* i, int v) {
  x64_put(i, v & 0xFF); x64_put(i, (v >> 8) & 0xFF);
  x64_put(i, (v >> 16) & 0xFF); x64_put(i, (v >> 24) & 0xFF);
}
inline void x64_put64(x64_insn* i, unsigned long long v) {
  for (int k = 0; k < 8; k++) { x64_put(i, (unsigned)(v & 0xFF)); v >>= 8; }
}

// REX prefix: 0x40 | W<<3 | R<<2 | X<<1 | B.  reg extends via R, rm/base
// via B.  Emitted when any bit is set or when `force` (byte ops on
// spl/bpl/sil/dil need an empty REX to mean those and not ah..bh).
inline void x64_rex(x64_insn* i, int w, int reg, int rm, bool force = false) {
  unsigned rex = 0x40u | ((w & 1) << 3) | (((reg >> 3) & 1) << 2) | ((rm >> 3) & 1);
  if (rex != 0x40u || force) x64_put(i, rex);
}

inline void x64_modrm(x64_insn* i, int mod, int reg, int rm) {
  x64_put(i, ((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7));
}

// ModRM(+SIB)(+disp) for a register-register operand: mod=11
inline void x64_modrm_rr(x64_insn* i, int reg, int rm) {
  x64_modrm(i, 3, reg, rm);
}

// ModRM(+SIB)(+disp) for [base + disp].  Handles the two irregular bases:
// rsp/r12 always need a SIB byte, and rbp/r13 cannot use the no-disp form
// (mod=00 rm=101 means rip-relative), so they get an explicit disp8 of 0.
inline void x64_modrm_mem(x64_insn* i, int reg, int base, int disp) {
  int b = base & 7;
  int mod;
  if (disp == 0 && b != 5)              mod = 0;
  else if (-128 <= disp && disp <= 127) mod = 1;
  else                                  mod = 2;
  x64_modrm(i, mod, reg, b);
  if (b == 4) x64_put(i, 0x24);         // SIB: scale=1, no index, base=rsp/r12
  if      (mod == 1) x64_put(i, disp & 0xFF);
  else if (mod == 2) x64_put32(i, disp);
}

// ModRM+disp32 for [rip + disp32]: mod=00, rm=101
inline void x64_modrm_rip(x64_insn* i, int reg, int disp32) {
  x64_modrm(i, 0, reg, 5);
  x64_put32(i, disp32);
}

// ---------------------------------------------------------------- loads/stores

// MOV r64, [base + disp]
inline x64_insn x64_mov_r_m(int rt, int base, int disp) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rt, base); x64_put(&i, 0x8B); x64_modrm_mem(&i, rt, base, disp);
  return i;
}
// MOV [base + disp], r64
inline x64_insn x64_mov_m_r(int base, int disp, int rt) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rt, base); x64_put(&i, 0x89); x64_modrm_mem(&i, rt, base, disp);
  return i;
}
// MOV r32, [base + disp] / MOV [base + disp], r32  (int32 counters and the like)
inline x64_insn x64_mov32_r_m(int rt, int base, int disp) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, rt, base); x64_put(&i, 0x8B); x64_modrm_mem(&i, rt, base, disp);
  return i;
}
inline x64_insn x64_mov32_m_r(int base, int disp, int rt) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, rt, base); x64_put(&i, 0x89); x64_modrm_mem(&i, rt, base, disp);
  return i;
}
// MOVZX r64, byte [base + disp]
inline x64_insn x64_movzxb_r_m(int rt, int base, int disp) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rt, base); x64_put(&i, 0x0F); x64_put(&i, 0xB6);
  x64_modrm_mem(&i, rt, base, disp);
  return i;
}
// MOV byte [base + disp], r8  (low byte of rt; REX forced for spl..dil)
inline x64_insn x64_mov_m_r8(int base, int disp, int rt) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, rt, base, /*force byte-reg REX*/ rt >= 4);
  x64_put(&i, 0x88); x64_modrm_mem(&i, rt, base, disp);
  return i;
}
// MOV qword [base + disp], imm32 (sign-extended; imm=0 is the str_zero analog)
inline x64_insn x64_mov_m_imm32(int base, int disp, int imm32) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, 0, base); x64_put(&i, 0xC7); x64_modrm_mem(&i, 0, base, disp);
  x64_put32(&i, imm32);
  return i;
}
// MOV dword [base + disp], imm32
inline x64_insn x64_mov32_m_imm32(int base, int disp, int imm32) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, 0, base); x64_put(&i, 0xC7); x64_modrm_mem(&i, 0, base, disp);
  x64_put32(&i, imm32);
  return i;
}
// MOV byte [base + disp], imm8
inline x64_insn x64_mov_m_imm8(int base, int disp, int imm8) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, 0, base); x64_put(&i, 0xC6); x64_modrm_mem(&i, 0, base, disp);
  x64_put(&i, imm8 & 0xFF);
  return i;
}
// MOV r64, [rip + disp32] -- the pc-relative literal load (cf. a64_ldr_lit)
inline x64_insn x64_mov_r_rip(int rt, int disp32) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rt, 0); x64_put(&i, 0x8B); x64_modrm_rip(&i, rt, disp32);
  return i;
}
// LEA r64, [rip + disp32] -- pc-relative address (cf. a64_adr)
inline x64_insn x64_lea_r_rip(int rt, int disp32) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rt, 0); x64_put(&i, 0x8D); x64_modrm_rip(&i, rt, disp32);
  return i;
}
// LEA r64, [base + disp]
inline x64_insn x64_lea_r_m(int rt, int base, int disp) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rt, base); x64_put(&i, 0x8D); x64_modrm_mem(&i, rt, base, disp);
  return i;
}

// ---------------------------------------------------------------- constants

// MOVABS r64, imm64
inline x64_insn x64_mov_r_imm64(int rd, unsigned long long v) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, 0, rd); x64_put(&i, 0xB8 + (rd & 7)); x64_put64(&i, v);
  return i;
}
// MOV r64, imm32 (sign-extended)
inline x64_insn x64_mov_r_imm32s(int rd, int imm32) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, 0, rd); x64_put(&i, 0xC7); x64_modrm_rr(&i, 0, rd);
  x64_put32(&i, imm32);
  return i;
}
// MOV r32, imm32 (zero-extends into the full register)
inline x64_insn x64_mov32_r_imm32(int rd, int imm32) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, 0, rd); x64_put(&i, 0xB8 + (rd & 7)); x64_put32(&i, imm32);
  return i;
}

// ---------------------------------------------------------------- arithmetic

// MOV/ADD/SUB/AND/OR/XOR/CMP/TEST rd, rs -- "op r/m, r" forms, reg=src rm=dst
// (the direction the system assembler picks, so the verifier matches)
inline x64_insn x64_op_r_r(unsigned opcode, int rd, int rs) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rs, rd); x64_put(&i, opcode); x64_modrm_rr(&i, rs, rd);
  return i;
}
inline x64_insn x64_mov_r_r (int rd, int rs) { return x64_op_r_r(0x89, rd, rs); }
inline x64_insn x64_add_r_r (int rd, int rs) { return x64_op_r_r(0x01, rd, rs); }
inline x64_insn x64_sub_r_r (int rd, int rs) { return x64_op_r_r(0x29, rd, rs); }
inline x64_insn x64_and_r_r (int rd, int rs) { return x64_op_r_r(0x21, rd, rs); }
inline x64_insn x64_or_r_r  (int rd, int rs) { return x64_op_r_r(0x09, rd, rs); }
inline x64_insn x64_xor_r_r (int rd, int rs) { return x64_op_r_r(0x31, rd, rs); }
inline x64_insn x64_cmp_r_r (int rd, int rs) { return x64_op_r_r(0x39, rd, rs); }
inline x64_insn x64_test_r_r(int rd, int rs) { return x64_op_r_r(0x85, rd, rs); }

// ADD/OR/AND/SUB/XOR/CMP rd, imm -- group-1 /digit; picks the imm8 form when
// it fits, the rax short form otherwise for rax (matching the assembler)
inline x64_insn x64_grp1_r_imm(int digit, int rd, int imm32) {
  x64_insn i = { 0, {} };
  if (-128 <= imm32 && imm32 <= 127) {
    x64_rex(&i, 1, 0, rd); x64_put(&i, 0x83); x64_modrm_rr(&i, digit, rd);
    x64_put(&i, imm32 & 0xFF);
  } else if (rd == x64_rax) {
    x64_rex(&i, 1, 0, 0); x64_put(&i, 0x05 + digit * 8); x64_put32(&i, imm32);
  } else {
    x64_rex(&i, 1, 0, rd); x64_put(&i, 0x81); x64_modrm_rr(&i, digit, rd);
    x64_put32(&i, imm32);
  }
  return i;
}
inline x64_insn x64_add_r_imm(int rd, int imm32) { return x64_grp1_r_imm(0, rd, imm32); }
inline x64_insn x64_or_r_imm (int rd, int imm32) { return x64_grp1_r_imm(1, rd, imm32); }
inline x64_insn x64_and_r_imm(int rd, int imm32) { return x64_grp1_r_imm(4, rd, imm32); }
inline x64_insn x64_sub_r_imm(int rd, int imm32) { return x64_grp1_r_imm(5, rd, imm32); }
inline x64_insn x64_xor_r_imm(int rd, int imm32) { return x64_grp1_r_imm(6, rd, imm32); }
inline x64_insn x64_cmp_r_imm(int rd, int imm32) { return x64_grp1_r_imm(7, rd, imm32); }

// TEST rd, imm32 (no imm8 form exists; rax has a short form)
inline x64_insn x64_test_r_imm(int rd, int imm32) {
  x64_insn i = { 0, {} };
  if (rd == x64_rax) {
    x64_rex(&i, 1, 0, 0); x64_put(&i, 0xA9); x64_put32(&i, imm32);
  } else {
    x64_rex(&i, 1, 0, rd); x64_put(&i, 0xF7); x64_modrm_rr(&i, 0, rd);
    x64_put32(&i, imm32);
  }
  return i;
}

// ADD dword [base + disp], imm  (invocation-counter bump)
inline x64_insn x64_add32_m_imm(int base, int disp, int imm32) {
  x64_insn i = { 0, {} };
  bool is8 = -128 <= imm32 && imm32 <= 127;
  x64_rex(&i, 0, 0, base); x64_put(&i, is8 ? 0x83 : 0x81);
  x64_modrm_mem(&i, 0, base, disp);
  if (is8) x64_put(&i, imm32 & 0xFF); else x64_put32(&i, imm32);
  return i;
}
// ADD qword [base + disp], imm  (e.g. NLR-adjusting the stacked return PC)
inline x64_insn x64_add64_m_imm(int base, int disp, int imm32) {
  x64_insn i = { 0, {} };
  bool is8 = -128 <= imm32 && imm32 <= 127;
  x64_rex(&i, 1, 0, base); x64_put(&i, is8 ? 0x83 : 0x81);
  x64_modrm_mem(&i, 0, base, disp);
  if (is8) x64_put(&i, imm32 & 0xFF); else x64_put32(&i, imm32);
  return i;
}
// CMP r64, [rip + disp32] -- compare against a literal-pool word (the word
// stays GC-updatable, so even moving oops can be compared without a scratch)
inline x64_insn x64_cmp_r_rip(int rt, int disp32) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rt, 0); x64_put(&i, 0x3B); x64_modrm_rip(&i, rt, disp32);
  return i;
}

// IMUL rd, rs
inline x64_insn x64_imul_r_r(int rd, int rs) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, rd, rs); x64_put(&i, 0x0F); x64_put(&i, 0xAF);
  x64_modrm_rr(&i, rd, rs);
  return i;
}
// CQO (sign-extend rax into rdx:rax, before IDIV)
inline x64_insn x64_cqo() {
  x64_insn i = { 0, {} };
  x64_put(&i, 0x48); x64_put(&i, 0x99);
  return i;
}
// IDIV r64 (rdx:rax / r -> quotient rax, remainder rdx)
inline x64_insn x64_idiv_r(int r) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, 0, r); x64_put(&i, 0xF7); x64_modrm_rr(&i, 7, r);
  return i;
}
// NEG r64
inline x64_insn x64_neg_r(int r) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, 0, r); x64_put(&i, 0xF7); x64_modrm_rr(&i, 3, r);
  return i;
}

// ---------------------------------------------------------------- shifts

// SHL/SHR/SAR rd, #sh  (group-2 /4,/5,/7; sh==1 uses the D1 short form)
inline x64_insn x64_shift_r_imm(int digit, int rd, int sh) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, 0, rd);
  if (sh == 1) { x64_put(&i, 0xD1); x64_modrm_rr(&i, digit, rd); }
  else { x64_put(&i, 0xC1); x64_modrm_rr(&i, digit, rd); x64_put(&i, sh & 0x3F); }
  return i;
}
inline x64_insn x64_shl_r_imm(int rd, int sh) { return x64_shift_r_imm(4, rd, sh); }
inline x64_insn x64_shr_r_imm(int rd, int sh) { return x64_shift_r_imm(5, rd, sh); }
inline x64_insn x64_sar_r_imm(int rd, int sh) { return x64_shift_r_imm(7, rd, sh); }
// SHL/SHR/SAR rd, cl
inline x64_insn x64_shift_r_cl(int digit, int rd) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 1, 0, rd); x64_put(&i, 0xD3); x64_modrm_rr(&i, digit, rd);
  return i;
}
inline x64_insn x64_shl_r_cl(int rd) { return x64_shift_r_cl(4, rd); }
inline x64_insn x64_shr_r_cl(int rd) { return x64_shift_r_cl(5, rd); }
inline x64_insn x64_sar_r_cl(int rd) { return x64_shift_r_cl(7, rd); }

// ---------------------------------------------------------------- branches

enum x64_cond {  // condition codes for jcc (0F 80+cc)
  x64_o = 0x0, x64_no = 0x1, x64_b  = 0x2, x64_ae = 0x3,
  x64_e = 0x4, x64_ne = 0x5, x64_be = 0x6, x64_a  = 0x7,
  x64_s = 0x8, x64_ns = 0x9, x64_p  = 0xA, x64_np = 0xB,
  x64_l = 0xC, x64_ge = 0xD, x64_le = 0xE, x64_g  = 0xF
};

// rel32 is measured from the END of the instruction, as the hardware does.
// JMP rel32 (never the rel8 form: branch sites must have a fixed shape)
inline x64_insn x64_jmp_rel32(int rel32) {
  x64_insn i = { 0, {} };
  x64_put(&i, 0xE9); x64_put32(&i, rel32);
  return i;
}
// Jcc rel32
inline x64_insn x64_jcc_rel32(x64_cond cc, int rel32) {
  x64_insn i = { 0, {} };
  x64_put(&i, 0x0F); x64_put(&i, 0x80 + (cc & 0xF)); x64_put32(&i, rel32);
  return i;
}
// CALL rel32
inline x64_insn x64_call_rel32(int rel32) {
  x64_insn i = { 0, {} };
  x64_put(&i, 0xE8); x64_put32(&i, rel32);
  return i;
}
// CALL/JMP r64
inline x64_insn x64_call_r(int r) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, 0, r); x64_put(&i, 0xFF); x64_modrm_rr(&i, 2, r);
  return i;
}
inline x64_insn x64_jmp_r(int r) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, 0, r); x64_put(&i, 0xFF); x64_modrm_rr(&i, 4, r);
  return i;
}
// CALL/JMP qword [rip + disp32] -- the send-site form: the target lives in a
// patchable 8-byte data word (cf. the aarch64 ldr x16/blr x16 + data word)
inline x64_insn x64_call_rip(int disp32) {
  x64_insn i = { 0, {} };
  x64_put(&i, 0xFF); x64_modrm_rip(&i, 2, disp32);
  return i;
}
inline x64_insn x64_jmp_rip(int disp32) {
  x64_insn i = { 0, {} };
  x64_put(&i, 0xFF); x64_modrm_rip(&i, 4, disp32);
  return i;
}

// ---------------------------------------------------------------- misc

inline x64_insn x64_push_r(int r) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, 0, r); x64_put(&i, 0x50 + (r & 7));
  return i;
}
inline x64_insn x64_pop_r(int r) {
  x64_insn i = { 0, {} };
  x64_rex(&i, 0, 0, r); x64_put(&i, 0x58 + (r & 7));
  return i;
}
inline x64_insn x64_ret() {
  x64_insn i = { 0, {} };
  x64_put(&i, 0xC3);
  return i;
}
inline x64_insn x64_leave() {
  x64_insn i = { 0, {} };
  x64_put(&i, 0xC9);
  return i;
}
inline x64_insn x64_int3() {  // breakpoint (cf. a64_brk)
  x64_insn i = { 0, {} };
  x64_put(&i, 0xCC);
  return i;
}
inline x64_insn x64_ud2() {   // guaranteed-undefined trap
  x64_insn i = { 0, {} };
  x64_put(&i, 0x0F); x64_put(&i, 0x0B);
  return i;
}
inline x64_insn x64_nop() {
  x64_insn i = { 0, {} };
  x64_put(&i, 0x90);
  return i;
}

// ------------------------------------------------- backpatching forward branches

// Classification of patchable instructions (the only forms our code
// generator ever emits against a forward Label).  Given the first byte of
// an emitted instruction, finds its rel32/disp32 field and its end (the
// point the offset is relative to).  Returns the field's address, or 0 if
// the bytes are not a patchable instruction; callers assert.
inline unsigned char* x64_patch_field(unsigned char* p, unsigned char** end) {
  unsigned char* q = p;
  if ((*q & 0xF0) == 0x40) q++;                    // skip REX
  if (*q == 0xE8 || *q == 0xE9) {                  // call/jmp rel32
    *end = q + 5;  return q + 1;
  }
  if (*q == 0x0F && (q[1] & 0xF0) == 0x80) {       // jcc rel32
    *end = q + 6;  return q + 2;
  }
  if (*q == 0xFF && (q[1] == 0x15 || q[1] == 0x25)) { // call/jmp [rip+d]
    *end = q + 6;  return q + 2;
  }
  if (q != p && (*q == 0x8B || *q == 0x8D || *q == 0x3B) && (q[1] & 0xC7) == 0x05) {
    *end = q + 6;  return q + 2;                   // REX mov/lea/cmp r,[rip+d]
  }
  return 0;
}

# endif // X64_ENCODING_HH
