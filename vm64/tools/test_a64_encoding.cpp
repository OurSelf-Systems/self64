// Standalone cross-check of vm64/src/aarch64/asm/a64_encoding.hh against
// a real assembler.  Driven by tools/test_a64_encoding.sh:
//   1. this program writes cases.s (assembly text) and ours.txt (our words)
//   2. the script assembles cases.s with clang and extracts the words
//   3. the script diffs the two lists
// Each CASE pairs one assembly line with one call into the encoding header.

# include <stdio.h>
# include "../src/aarch64/asm/a64_encoding.hh"

static FILE* sfile;
static FILE* wfile;

static void CASE(const char* text, a64_inst word) {
  fprintf(sfile, "  %s\n", text);
  fprintf(wfile, "%08x %s\n", word, text);
}

static void logical_imm_case(const char* mnem,
                             a64_inst (*enc)(int, int, int, int, int),
                             int rd, int rn, unsigned long long v) {
  int n, immr, imms;
  if (!a64_bitmask_imm(v, &n, &immr, &imms)) {
    fprintf(stderr, "bitmask_imm failed for %#llx\n", v);
    return;
  }
  char buf[128];
  snprintf(buf, sizeof buf, "%s x%d, x%d, #0x%llx", mnem, rd, rn, v);
  CASE(buf, enc(rd, rn, n, immr, imms));
}

int main() {
  sfile = fopen("cases.s", "w");
  wfile = fopen("ours.txt", "w");
  fprintf(sfile, "  .text\n  .align 2\n");

  // loads/stores
  CASE("ldr x3, [x9, #0]",        a64_ldr_uoff(3, 9, 0));
  CASE("ldr x3, [x9, #8]",        a64_ldr_uoff(3, 9, 1));
  CASE("ldr x0, [sp, #32760]",    a64_ldr_uoff(0, 31, 4095));
  CASE("str x3, [x9, #16]",       a64_str_uoff(3, 9, 2));
  CASE("str x30, [x29, #8]",      a64_str_uoff(30, 29, 1));
  CASE("ldur x3, [x9, #-8]",      a64_ldur(3, 9, -8));
  CASE("ldur x3, [x9, #255]",     a64_ldur(3, 9, 255));
  CASE("stur x12, [x29, #-256]",  a64_stur(12, 29, -256));
  CASE("ldr x5, . + 16",          a64_ldr_lit(5, 4));
  CASE("ldr x16, . - 4",          a64_ldr_lit(16, -1));
  CASE("ldr w4, [x9, #8]",        a64_ldrw_uoff(4, 9, 2));
  CASE("str w4, [x9, #8]",        a64_strw_uoff(4, 9, 2));
  CASE("add w4, w4, #1",          a64_addw_imm(4, 4, 1));
  CASE("ldrb w4, [x9, #3]",       a64_ldrb_uoff(4, 9, 3));
  CASE("strb w4, [x9, #3]",       a64_strb_uoff(4, 9, 3));

  // move wide
  CASE("movz x7, #0x1234",            a64_movz(7, 0x1234, 0));
  CASE("movz x7, #0xffff, lsl #48",   a64_movz(7, 0xFFFF, 3));
  CASE("movk x7, #0xbeef, lsl #16",   a64_movk(7, 0xBEEF, 1));
  CASE("movk x7, #0x1, lsl #32",      a64_movk(7, 1, 2));
  CASE("movn x7, #0x2, lsl #16",      a64_movn(7, 2, 1));

  // arithmetic
  CASE("add x1, x2, #42",             a64_add_imm(1, 2, 42));
  CASE("add x1, x2, #42, lsl #12",    a64_add_imm(1, 2, 42, true));
  CASE("add x1, sp, #0",              a64_add_imm(1, 31, 0));
  CASE("adds x1, x2, #7",             a64_add_imm(1, 2, 7, false, true));
  CASE("sub x1, x2, #42",             a64_sub_imm(1, 2, 42));
  CASE("subs x1, x2, #42",            a64_sub_imm(1, 2, 42, false, true));
  CASE("cmp x9, #4095",               a64_cmp_imm(9, 4095));
  CASE("add x1, x2, x3",              a64_add_reg(1, 2, 3));
  CASE("add x1, x2, x3, lsl #4",      a64_add_reg(1, 2, 3, 4));
  CASE("adds x1, x2, x3",             a64_add_reg(1, 2, 3, 0, true));
  CASE("sub x1, x2, x3",              a64_sub_reg(1, 2, 3));
  CASE("subs x1, x2, x3, lsl #1",     a64_sub_reg(1, 2, 3, 1, true));
  CASE("cmp x9, x10",                 a64_cmp_reg(9, 10));
  CASE("mul x4, x5, x6",              a64_mul(4, 5, 6));
  CASE("sdiv x4, x5, x6",             a64_sdiv(4, 5, 6));

  // logical (register)
  CASE("and x1, x2, x3",              a64_and_reg(1, 2, 3));
  CASE("orr x1, x2, x3",              a64_orr_reg(1, 2, 3));
  CASE("orr x1, x2, x3, lsl #2",      a64_orr_reg(1, 2, 3, 2));
  CASE("eor x1, x2, x3",              a64_eor_reg(1, 2, 3));
  CASE("ands x1, x2, x3",             a64_ands_reg(1, 2, 3));
  CASE("mov x4, x5",                  a64_mov_reg(4, 5));
  CASE("mov x4, sp",                  a64_mov_sp(4, 31));
  CASE("mov sp, x4",                  a64_mov_sp(31, 4));

  // logical (bitmask immediate) -- exercises a64_bitmask_imm
  logical_imm_case("and",  a64_and_imm,  1, 2, 0x3);                  // Tag_Mask
  logical_imm_case("and",  a64_and_imm,  1, 2, 0x1);
  logical_imm_case("ands", a64_ands_imm, 1, 2, 0x3);
  logical_imm_case("and",  a64_and_imm,  1, 2, 0xFF00);
  logical_imm_case("orr",  a64_orr_imm,  1, 2, 0x7FFFFFFFULL);
  logical_imm_case("eor",  a64_eor_imm,  1, 2, 0xFFFFFFFF00000000ULL);
  logical_imm_case("and",  a64_and_imm,  1, 2, 0x5555555555555555ULL);
  logical_imm_case("and",  a64_and_imm,  1, 2, 0x0F0F0F0F0F0F0F0FULL);
  logical_imm_case("and",  a64_and_imm,  1, 2, 0x8000000000000001ULL); // wraps
  logical_imm_case("and",  a64_and_imm,  1, 2, 0xFFFCULL);
  logical_imm_case("and",  a64_and_imm,  1, 2, 0xFFFFFFFFFFFFFFF0ULL);
  logical_imm_case("orr",  a64_orr_imm,  3, 4, 0x0001000100010001ULL);

  // shifts
  CASE("lsl x1, x2, #3",              a64_lsl_imm(1, 2, 3));
  CASE("lsl x1, x2, #63",             a64_lsl_imm(1, 2, 63));
  CASE("lsr x1, x2, #3",              a64_lsr_imm(1, 2, 3));
  CASE("asr x1, x2, #2",              a64_asr_imm(1, 2, 2));

  // branches
  CASE("b . + 8",                     a64_b(2));
  CASE("b . - 64",                    a64_b(-16));
  CASE("bl . + 4096",                 a64_bl(1024));
  CASE("b.eq . + 8",                  a64_bcond(a64_eq, 2));
  CASE("b.ne . - 8",                  a64_bcond(a64_ne, -2));
  CASE("b.lt . + 0",                  a64_bcond(a64_lt, 0));
  CASE("b.ge . + 32",                 a64_bcond(a64_ge, 8));
  CASE("cbz x3, . + 16",              a64_cbz(3, 4));
  CASE("cbnz x3, . - 16",             a64_cbnz(3, -4));
  CASE("br x16",                      a64_br(16));
  CASE("blr x16",                     a64_blr(16));
  CASE("ret",                         a64_ret());

  // misc
  CASE("brk #0x123",                  a64_brk(0x123));
  CASE("nop",                         a64_nop());
  CASE("stp x29, x30, [sp, #-16]",    a64_stp(29, 30, 31, -2));
  CASE("stp x29, x30, [x9, #16]",     a64_stp(29, 30, 9, 2));
  CASE("ldp x29, x30, [sp, #32]",     a64_ldp(29, 30, 31, 4));

  fclose(sfile);
  fclose(wfile);

  // self-test the patcher: patch zero-offset branches to known offsets
  int fails = 0;
  if (a64_patch_branch(a64_b(0), 5)        != a64_b(5))        { fprintf(stderr, "patch b failed\n"); fails++; }
  if (a64_patch_branch(a64_bl(0), -3)      != a64_bl(-3))      { fprintf(stderr, "patch bl failed\n"); fails++; }
  if (a64_patch_branch(a64_bcond(a64_le, 0), 7) != a64_bcond(a64_le, 7)) { fprintf(stderr, "patch b.cond failed\n"); fails++; }
  if (a64_patch_branch(a64_cbz(9, 0), 7)   != a64_cbz(9, 7))   { fprintf(stderr, "patch cbz failed\n"); fails++; }
  if (a64_patch_branch(a64_ldr_lit(5, 0), 9) != a64_ldr_lit(5, 9)) { fprintf(stderr, "patch ldr-lit failed\n"); fails++; }
  if (fails) return 1;
  fprintf(stderr, "patch self-test ok\n");
  return 0;
}
