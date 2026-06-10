/* Copyright 2026 AUTHORS.
   See the LICENSE file for license information. */

// Pure A64 instruction encoding helpers.
//
// Deliberately free of VM dependencies (plain unsigned types, no oops, no
// Locations) so the encodings can be verified standalone against a real
// assembler -- see vm64/tools/test_a64_encoding.sh, which assembles the
// same forms with clang and compares words.  All forms are 64-bit (sf=1)
// unless noted.  Register numbers are hardware numbers 0..30, with 31
// meaning SP or XZR depending on the instruction class, as in the ARM ARM.

# ifndef A64_ENCODING_HH
# define A64_ENCODING_HH

typedef unsigned int a64_inst;   // one 32-bit instruction word

const int a64_xzr = 31;          // also encodes SP in base-register fields

// ---------------------------------------------------------------- loads/stores

// LDR Xt, [Xn, #pimm]  -- pimm = uimm12 * 8, unsigned scaled offset
inline a64_inst a64_ldr_uoff(int rt, int rn, unsigned uimm12) {
  return 0xF9400000u | ((uimm12 & 0xFFFu) << 10) | ((rn & 31) << 5) | (rt & 31);
}
// STR Xt, [Xn, #pimm]
inline a64_inst a64_str_uoff(int rt, int rn, unsigned uimm12) {
  return 0xF9000000u | ((uimm12 & 0xFFFu) << 10) | ((rn & 31) << 5) | (rt & 31);
}
// LDUR Xt, [Xn, #simm9]  -- unscaled, for negative/unaligned offsets
inline a64_inst a64_ldur(int rt, int rn, int simm9) {
  return 0xF8400000u | ((simm9 & 0x1FF) << 12) | ((rn & 31) << 5) | (rt & 31);
}
// STUR Xt, [Xn, #simm9]
inline a64_inst a64_stur(int rt, int rn, int simm9) {
  return 0xF8000000u | ((simm9 & 0x1FF) << 12) | ((rn & 31) << 5) | (rt & 31);
}
// LDR Xt, label  -- pc-relative literal load, offset_words = (target-pc)/4
inline a64_inst a64_ldr_lit(int rt, int offset_words) {
  return 0x58000000u | ((offset_words & 0x7FFFF) << 5) | (rt & 31);
}
// LDR Wt, [Xn, #uimm12*4] / STR -- 32-bit forms (e.g. int32 counters)
inline a64_inst a64_ldrw_uoff(int rt, int rn, unsigned uimm12) {
  return 0xB9400000u | ((uimm12 & 0xFFFu) << 10) | ((rn & 31) << 5) | (rt & 31);
}
inline a64_inst a64_strw_uoff(int rt, int rn, unsigned uimm12) {
  return 0xB9000000u | ((uimm12 & 0xFFFu) << 10) | ((rn & 31) << 5) | (rt & 31);
}
// ADD Wd, Wn, #imm12 -- 32-bit add for counter bumps
inline a64_inst a64_addw_imm(int rd, int rn, unsigned imm12) {
  return 0x11000000u | ((imm12 & 0xFFFu) << 10) | ((rn & 31) << 5) | (rd & 31);
}

// LDRB Wt, [Xn, #uimm12] / STRB
inline a64_inst a64_ldrb_uoff(int rt, int rn, unsigned uimm12) {
  return 0x39400000u | ((uimm12 & 0xFFFu) << 10) | ((rn & 31) << 5) | (rt & 31);
}
inline a64_inst a64_strb_uoff(int rt, int rn, unsigned uimm12) {
  return 0x39000000u | ((uimm12 & 0xFFFu) << 10) | ((rn & 31) << 5) | (rt & 31);
}

// ---------------------------------------------------------------- move wide

// MOVZ Xd, #imm16, LSL #(hw*16)
inline a64_inst a64_movz(int rd, unsigned imm16, int hw) {
  return 0xD2800000u | ((hw & 3) << 21) | ((imm16 & 0xFFFFu) << 5) | (rd & 31);
}
// MOVK Xd, #imm16, LSL #(hw*16)
inline a64_inst a64_movk(int rd, unsigned imm16, int hw) {
  return 0xF2800000u | ((hw & 3) << 21) | ((imm16 & 0xFFFFu) << 5) | (rd & 31);
}
// MOVN Xd, #imm16, LSL #(hw*16)
inline a64_inst a64_movn(int rd, unsigned imm16, int hw) {
  return 0x92800000u | ((hw & 3) << 21) | ((imm16 & 0xFFFFu) << 5) | (rd & 31);
}

// ---------------------------------------------------------------- arithmetic

// ADD/SUB Xd, Xn, #imm12 [, LSL #12]; setflags makes ADDS/SUBS (CMP = SUBS xzr)
inline a64_inst a64_add_imm(int rd, int rn, unsigned imm12, bool shift12 = false,
                            bool setflags = false) {
  return (setflags ? 0xB1000000u : 0x91000000u)
       | ((shift12 ? 1u : 0u) << 22) | ((imm12 & 0xFFFu) << 10)
       | ((rn & 31) << 5) | (rd & 31);
}
inline a64_inst a64_sub_imm(int rd, int rn, unsigned imm12, bool shift12 = false,
                            bool setflags = false) {
  return (setflags ? 0xF1000000u : 0xD1000000u)
       | ((shift12 ? 1u : 0u) << 22) | ((imm12 & 0xFFFu) << 10)
       | ((rn & 31) << 5) | (rd & 31);
}
// ADD/SUB Xd, Xn, Xm (shifted register form, LSL #amount)
inline a64_inst a64_add_reg(int rd, int rn, int rm, int lsl = 0,
                            bool setflags = false) {
  return (setflags ? 0xAB000000u : 0x8B000000u)
       | ((rm & 31) << 16) | ((lsl & 63) << 10) | ((rn & 31) << 5) | (rd & 31);
}
inline a64_inst a64_sub_reg(int rd, int rn, int rm, int lsl = 0,
                            bool setflags = false) {
  return (setflags ? 0xEB000000u : 0xCB000000u)
       | ((rm & 31) << 16) | ((lsl & 63) << 10) | ((rn & 31) << 5) | (rd & 31);
}
// CMP Xn, Xm / CMP Xn, #imm12
inline a64_inst a64_cmp_reg(int rn, int rm) { return a64_sub_reg(a64_xzr, rn, rm, 0, true); }
inline a64_inst a64_cmp_imm(int rn, unsigned imm12) { return a64_sub_imm(a64_xzr, rn, imm12, false, true); }

// MUL Xd, Xn, Xm  (MADD with xzr accumulator)
inline a64_inst a64_mul(int rd, int rn, int rm) {
  return 0x9B007C00u | ((rm & 31) << 16) | ((rn & 31) << 5) | (rd & 31);
}
// SDIV Xd, Xn, Xm
inline a64_inst a64_sdiv(int rd, int rn, int rm) {
  return 0x9AC00C00u | ((rm & 31) << 16) | ((rn & 31) << 5) | (rd & 31);
}

// ---------------------------------------------------------------- logical

// AND/ORR/EOR/ANDS Xd, Xn, Xm (shifted register, LSL #amount)
inline a64_inst a64_and_reg(int rd, int rn, int rm, int lsl = 0) {
  return 0x8A000000u | ((rm & 31) << 16) | ((lsl & 63) << 10) | ((rn & 31) << 5) | (rd & 31);
}
inline a64_inst a64_orr_reg(int rd, int rn, int rm, int lsl = 0) {
  return 0xAA000000u | ((rm & 31) << 16) | ((lsl & 63) << 10) | ((rn & 31) << 5) | (rd & 31);
}
inline a64_inst a64_eor_reg(int rd, int rn, int rm, int lsl = 0) {
  return 0xCA000000u | ((rm & 31) << 16) | ((lsl & 63) << 10) | ((rn & 31) << 5) | (rd & 31);
}
inline a64_inst a64_ands_reg(int rd, int rn, int rm, int lsl = 0) {
  return 0xEA000000u | ((rm & 31) << 16) | ((lsl & 63) << 10) | ((rn & 31) << 5) | (rd & 31);
}
// MOV Xd, Xm  (ORR Xd, XZR, Xm)
inline a64_inst a64_mov_reg(int rd, int rm) { return a64_orr_reg(rd, a64_xzr, rm); }
// MOV to/from SP must use ADD #0 (ORR can't address SP)
inline a64_inst a64_mov_sp(int rd, int rn) { return a64_add_imm(rd, rn, 0); }

// Bitmask-immediate encoding for 64-bit logical-immediate instructions.
// Returns false if value is not encodable (0 and ~0 never are).
// On success fills n/immr/imms.  Uses the canonical scheme: the decoded
// value is ROR(ones-in-LSBs, immr) replicated to 64 bits; we find the
// element size by halving and the rotation by brute force (<= 64 tries).
inline bool a64_bitmask_imm(unsigned long long v, int* n, int* immr, int* imms) {
  if (v == 0 || ~v == 0) return false;
  int size = 64;
  while (size > 2) {
    int half = size >> 1;
    unsigned long long mask = (1ULL << half) - 1;
    if ((v & mask) != ((v >> half) & mask)) break;
    size = half;
  }
  unsigned long long emask = (size == 64) ? ~0ULL : ((1ULL << size) - 1);
  unsigned long long elem = v & emask;
  int ones = 0;
  for (int i = 0; i < size; i++) if ((elem >> i) & 1) ones++;
  if (ones == 0 || ones == size) return false;
  unsigned long long base = (1ULL << ones) - 1;  // ones in LSBs
  int rot = -1;
  for (int r = 0; r < size; r++) {
    unsigned long long ror = ((base >> r) | (base << (size - r))) & emask;
    if (r == 0) ror = base;
    if (ror == elem) { rot = r; break; }
  }
  if (rot < 0) return false;  // ones not contiguous (mod rotation)
  *n    = (size == 64) ? 1 : 0;
  *immr = rot;
  *imms = (int)((( ~(unsigned)(size - 1) << 1 ) | (unsigned)(ones - 1)) & 0x3F);
  return true;
}

// AND/ORR/EOR/ANDS Xd, Xn, #bitmask -- call a64_bitmask_imm first
inline a64_inst a64_and_imm(int rd, int rn, int n, int immr, int imms) {
  return 0x92000000u | ((n & 1) << 22) | ((immr & 0x3F) << 16) | ((imms & 0x3F) << 10)
       | ((rn & 31) << 5) | (rd & 31);
}
inline a64_inst a64_orr_imm(int rd, int rn, int n, int immr, int imms) {
  return 0xB2000000u | ((n & 1) << 22) | ((immr & 0x3F) << 16) | ((imms & 0x3F) << 10)
       | ((rn & 31) << 5) | (rd & 31);
}
inline a64_inst a64_eor_imm(int rd, int rn, int n, int immr, int imms) {
  return 0xD2000000u | ((n & 1) << 22) | ((immr & 0x3F) << 16) | ((imms & 0x3F) << 10)
       | ((rn & 31) << 5) | (rd & 31);
}
inline a64_inst a64_ands_imm(int rd, int rn, int n, int immr, int imms) {
  return 0xF2000000u | ((n & 1) << 22) | ((immr & 0x3F) << 16) | ((imms & 0x3F) << 10)
       | ((rn & 31) << 5) | (rd & 31);
}

// ---------------------------------------------------------------- shifts

// LSL Xd, Xn, #sh  (UBFM Xd, Xn, #((64-sh)%64), #(63-sh))
inline a64_inst a64_lsl_imm(int rd, int rn, int sh) {
  int immr = (64 - sh) & 63, imms = 63 - sh;
  return 0xD3400000u | ((immr & 0x3F) << 16) | ((imms & 0x3F) << 10)
       | ((rn & 31) << 5) | (rd & 31);
}
// LSR Xd, Xn, #sh  (UBFM Xd, Xn, #sh, #63)
inline a64_inst a64_lsr_imm(int rd, int rn, int sh) {
  return 0xD3400000u | ((sh & 0x3F) << 16) | (63u << 10)
       | ((rn & 31) << 5) | (rd & 31);
}
// ASR Xd, Xn, #sh  (SBFM Xd, Xn, #sh, #63)
inline a64_inst a64_asr_imm(int rd, int rn, int sh) {
  return 0x93400000u | ((sh & 0x3F) << 16) | (63u << 10)
       | ((rn & 31) << 5) | (rd & 31);
}

// ---------------------------------------------------------------- branches

enum a64_cond {  // condition codes for b.cond
  a64_eq = 0x0, a64_ne = 0x1, a64_cs = 0x2, a64_cc = 0x3,
  a64_mi = 0x4, a64_pl = 0x5, a64_vs = 0x6, a64_vc = 0x7,
  a64_hi = 0x8, a64_ls = 0x9, a64_ge = 0xA, a64_lt = 0xB,
  a64_gt = 0xC, a64_le = 0xD, a64_al = 0xE
};

// B label; offset_words = (target - pc) / 4
inline a64_inst a64_b(int offset_words) {
  return 0x14000000u | (offset_words & 0x03FFFFFF);
}
// BL label
inline a64_inst a64_bl(int offset_words) {
  return 0x94000000u | (offset_words & 0x03FFFFFF);
}
// B.cond label
inline a64_inst a64_bcond(a64_cond cond, int offset_words) {
  return 0x54000000u | ((offset_words & 0x7FFFF) << 5) | (cond & 0xF);
}
// CBZ/CBNZ Xt, label
inline a64_inst a64_cbz(int rt, int offset_words) {
  return 0xB4000000u | ((offset_words & 0x7FFFF) << 5) | (rt & 31);
}
inline a64_inst a64_cbnz(int rt, int offset_words) {
  return 0xB5000000u | ((offset_words & 0x7FFFF) << 5) | (rt & 31);
}
// BR/BLR Xn, RET {Xn}
inline a64_inst a64_br (int rn) { return 0xD61F0000u | ((rn & 31) << 5); }
inline a64_inst a64_blr(int rn) { return 0xD63F0000u | ((rn & 31) << 5); }
inline a64_inst a64_ret(int rn = 30) { return 0xD65F0000u | ((rn & 31) << 5); }

// ---------------------------------------------------------------- misc

inline a64_inst a64_brk(unsigned imm16) { return 0xD4200000u | ((imm16 & 0xFFFFu) << 5); }
inline a64_inst a64_nop() { return 0xD503201Fu; }

// STP/LDP Xt1, Xt2, [Xn, #simm7*8]  -- prologue/epilogue frame links
inline a64_inst a64_stp(int rt1, int rt2, int rn, int simm7_scaled) {
  return 0xA9000000u | ((simm7_scaled & 0x7F) << 15) | ((rt2 & 31) << 10)
       | ((rn & 31) << 5) | (rt1 & 31);
}
inline a64_inst a64_ldp(int rt1, int rt2, int rn, int simm7_scaled) {
  return 0xA9400000u | ((simm7_scaled & 0x7F) << 15) | ((rt2 & 31) << 10)
       | ((rn & 31) << 5) | (rt1 & 31);
}

// ------------------------------------------------- backpatching forward branches

// Classification of patchable instruction words (the only forms our code
// generator ever emits with a forward Label).
inline bool a64_is_b_or_bl(a64_inst w)   { return (w & 0x7C000000u) == 0x14000000u; }
inline bool a64_is_bcond(a64_inst w)     { return (w & 0xFF000010u) == 0x54000000u; }
inline bool a64_is_cb(a64_inst w)        { return (w & 0x7E000000u) == 0x34000000u; }
inline bool a64_is_ldr_lit(a64_inst w)   { return (w & 0xBF000000u) == 0x18000000u; }

// Insert a branch offset (in words) into an already-emitted instruction.
// Returns the patched word; callers store it back and need no icache flush
// here because patching happens before the buffer is copied into the zone.
inline a64_inst a64_patch_branch(a64_inst w, int offset_words) {
  if (a64_is_b_or_bl(w))
    return (w & 0xFC000000u) | (offset_words & 0x03FFFFFF);
  if (a64_is_bcond(w))
    return (w & 0xFF00001Fu) | ((offset_words & 0x7FFFF) << 5);
  if (a64_is_cb(w) || a64_is_ldr_lit(w))
    return (w & 0xFF00001Fu) | ((offset_words & 0x7FFFF) << 5);
  return 0; // not a patchable instruction; caller asserts
}

# endif // A64_ENCODING_HH
