// Standalone cross-check of vm64/src/amd64/asm/x64_encoding.hh against
// a real assembler.  Driven by tools/test_x64_encoding.sh:
//   1. this program writes cases.s (Intel-syntax assembly) and ours.txt
//      (our encoded bytes, one hex string per case)
//   2. the script assembles cases.s and extracts the bytes per instruction
//   3. the script diffs the two lists
// Each CASE pairs one assembly line with one call into the encoding header.

# include <stdio.h>
# include "../src/amd64/asm/x64_encoding.hh"

static FILE* sfile;
static FILE* wfile;

static void CASE(const char* text, x64_insn i) {
  fprintf(sfile, "  %s\n", text);
  for (int k = 0; k < i.len; k++) fprintf(wfile, "%02x", i.b[k]);
  fprintf(wfile, " %s\n", text);
}

int main() {
  sfile = fopen("cases.s", "w");
  wfile = fopen("ours.txt", "w");
  fprintf(sfile, "  .intel_syntax noprefix\n  .text\n");

  // loads/stores: plain bases, disp8/disp32, and the irregular encodings --
  // rsp/r12 need a SIB byte, rbp/r13 need an explicit disp
  CASE("mov rax, [rcx]",             x64_mov_r_m(x64_rax, x64_rcx, 0));
  CASE("mov rax, [rcx+8]",           x64_mov_r_m(x64_rax, x64_rcx, 8));
  CASE("mov rax, [rcx-8]",           x64_mov_r_m(x64_rax, x64_rcx, -8));
  CASE("mov rax, [rcx+0x12345]",     x64_mov_r_m(x64_rax, x64_rcx, 0x12345));
  CASE("mov rax, [rsp+8]",           x64_mov_r_m(x64_rax, x64_rsp, 8));
  CASE("mov rax, [rsp]",             x64_mov_r_m(x64_rax, x64_rsp, 0));
  CASE("mov rax, [rbp]",             x64_mov_r_m(x64_rax, x64_rbp, 0));
  CASE("mov rax, [rbp-24]",          x64_mov_r_m(x64_rax, x64_rbp, -24));
  CASE("mov rax, [rbp+0x12345]",     x64_mov_r_m(x64_rax, x64_rbp, 0x12345));
  CASE("mov r9, [r12+16]",           x64_mov_r_m(x64_r9,  x64_r12, 16));
  CASE("mov r9, [r13]",              x64_mov_r_m(x64_r9,  x64_r13, 0));
  CASE("mov r15, [r8-128]",          x64_mov_r_m(x64_r15, x64_r8, -128));
  CASE("mov [rcx+8], rax",           x64_mov_m_r(x64_rcx, 8, x64_rax));
  CASE("mov [rbp-8], r10",           x64_mov_m_r(x64_rbp, -8, x64_r10));
  CASE("mov [rsp+128], rsi",         x64_mov_m_r(x64_rsp, 128, x64_rsi));
  CASE("mov eax, [rcx+8]",           x64_mov32_r_m(x64_rax, x64_rcx, 8));
  CASE("mov r9d, [rbp-4]",           x64_mov32_r_m(x64_r9, x64_rbp, -4));
  CASE("mov [rcx+8], eax",           x64_mov32_m_r(x64_rcx, 8, x64_rax));
  CASE("movzx rax, byte ptr [rcx+3]", x64_movzxb_r_m(x64_rax, x64_rcx, 3));
  CASE("movzx r9, byte ptr [rbp-1]", x64_movzxb_r_m(x64_r9, x64_rbp, -1));
  CASE("mov byte ptr [rcx+3], al",   x64_mov_m_r8(x64_rcx, 3, x64_rax));
  CASE("mov byte ptr [rcx+3], sil",  x64_mov_m_r8(x64_rcx, 3, x64_rsi));
  CASE("mov byte ptr [rcx+3], r9b",  x64_mov_m_r8(x64_rcx, 3, x64_r9));
  CASE("mov qword ptr [rcx+8], 0",   x64_mov_m_imm32(x64_rcx, 8, 0));
  CASE("mov qword ptr [rbp-16], 42", x64_mov_m_imm32(x64_rbp, -16, 42));
  CASE("mov dword ptr [rcx+8], 7",   x64_mov32_m_imm32(x64_rcx, 8, 7));
  CASE("mov byte ptr [rcx+3], 1",    x64_mov_m_imm8(x64_rcx, 3, 1));
  CASE("mov rax, [rip+0x100]",       x64_mov_r_rip(x64_rax, 0x100));
  CASE("mov r11, [rip-0x20]",        x64_mov_r_rip(x64_r11, -0x20));
  CASE("lea rax, [rip+0x40]",        x64_lea_r_rip(x64_rax, 0x40));
  CASE("lea rcx, [rbp-32]",          x64_lea_r_m(x64_rcx, x64_rbp, -32));
  CASE("lea rsi, [rsp+8]",           x64_lea_r_m(x64_rsi, x64_rsp, 8));

  // constants
  CASE("movabs rax, 0x123456789abcdef0", x64_mov_r_imm64(x64_rax, 0x123456789ABCDEF0ULL));
  CASE("movabs r15, 0xdeadbeef00000000", x64_mov_r_imm64(x64_r15, 0xDEADBEEF00000000ULL));
  CASE("mov rax, 42",                x64_mov_r_imm32s(x64_rax, 42));
  CASE("mov r9, -1",                 x64_mov_r_imm32s(x64_r9, -1));
  CASE("mov eax, 42",                x64_mov32_r_imm32(x64_rax, 42));
  CASE("mov r9d, 0x12345",           x64_mov32_r_imm32(x64_r9, 0x12345));

  // register-register moves and arithmetic
  CASE("mov rax, rbx",               x64_mov_r_r(x64_rax, x64_rbx));
  CASE("mov r9, rsp",                x64_mov_r_r(x64_r9, x64_rsp));
  CASE("mov rbp, rsp",               x64_mov_r_r(x64_rbp, x64_rsp));
  CASE("add rcx, rdx",               x64_add_r_r(x64_rcx, x64_rdx));
  CASE("sub rcx, r9",                x64_sub_r_r(x64_rcx, x64_r9));
  CASE("and r10, rcx",               x64_and_r_r(x64_r10, x64_rcx));
  CASE("or rcx, rdx",                x64_or_r_r(x64_rcx, x64_rdx));
  CASE("xor rcx, rdx",               x64_xor_r_r(x64_rcx, x64_rdx));
  CASE("cmp rcx, rdx",               x64_cmp_r_r(x64_rcx, x64_rdx));
  CASE("test rcx, rdx",              x64_test_r_r(x64_rcx, x64_rdx));

  // group-1 immediates: imm8 form, rax short form, generic imm32 form
  CASE("add rcx, 5",                 x64_add_r_imm(x64_rcx, 5));
  CASE("add rax, 5",                 x64_add_r_imm(x64_rax, 5));
  CASE("add rax, 0x12345",           x64_add_r_imm(x64_rax, 0x12345));
  CASE("add rcx, 0x12345",           x64_add_r_imm(x64_rcx, 0x12345));
  CASE("add r12, -16",               x64_add_r_imm(x64_r12, -16));
  CASE("sub rsp, 0x180",             x64_sub_r_imm(x64_rsp, 0x180));
  CASE("sub rsp, 16",                x64_sub_r_imm(x64_rsp, 16));
  CASE("and rcx, 3",                 x64_and_r_imm(x64_rcx, 3));
  CASE("and rax, 0x7fffffff",        x64_and_r_imm(x64_rax, 0x7FFFFFFF));
  CASE("or rcx, 1",                  x64_or_r_imm(x64_rcx, 1));
  CASE("xor rcx, -1",                x64_xor_r_imm(x64_rcx, -1));
  CASE("cmp rcx, 0",                 x64_cmp_r_imm(x64_rcx, 0));
  CASE("cmp rax, 0x1000",            x64_cmp_r_imm(x64_rax, 0x1000));
  CASE("cmp r11, 0x1000",            x64_cmp_r_imm(x64_r11, 0x1000));
  CASE("test rax, 0x100",            x64_test_r_imm(x64_rax, 0x100));
  CASE("test rcx, 3",                x64_test_r_imm(x64_rcx, 3));
  CASE("add dword ptr [rcx+8], 1",   x64_add32_m_imm(x64_rcx, 8, 1));
  CASE("add dword ptr [rbp-4], 0x200", x64_add32_m_imm(x64_rbp, -4, 0x200));
  CASE("add qword ptr [rsp], 5",     x64_add64_m_imm(x64_rsp, 0, 5));
  CASE("add qword ptr [rbp-8], 0x200", x64_add64_m_imm(x64_rbp, -8, 0x200));
  CASE("cmp rax, [rip+0x30]",        x64_cmp_r_rip(x64_rax, 0x30));
  CASE("cmp r9, [rip-0x10]",         x64_cmp_r_rip(x64_r9, -0x10));

  // multiply/divide/negate
  CASE("imul rcx, rdx",              x64_imul_r_r(x64_rcx, x64_rdx));
  CASE("imul r9, r10",               x64_imul_r_r(x64_r9, x64_r10));
  CASE("cqo",                        x64_cqo());
  CASE("idiv rcx",                   x64_idiv_r(x64_rcx));
  CASE("idiv r9",                    x64_idiv_r(x64_r9));
  CASE("neg rax",                    x64_neg_r(x64_rax));

  // shifts: the sh==1 short form and the generic form, plus by-cl
  CASE("shl rax, 1",                 x64_shl_r_imm(x64_rax, 1));
  CASE("shl rax, 5",                 x64_shl_r_imm(x64_rax, 5));
  CASE("shl r9, 63",                 x64_shl_r_imm(x64_r9, 63));
  CASE("shr rax, 2",                 x64_shr_r_imm(x64_rax, 2));
  CASE("sar rax, 1",                 x64_sar_r_imm(x64_rax, 1));
  CASE("sar r12, 3",                 x64_sar_r_imm(x64_r12, 3));
  CASE("shl rax, cl",                x64_shl_r_cl(x64_rax));
  CASE("shr r9, cl",                 x64_shr_r_cl(x64_r9));
  CASE("sar rdx, cl",                x64_sar_r_cl(x64_rdx));

  // branches: distances beyond rel8 range so the assembler picks rel32 too.
  // rel32 is from instruction end: "jmp . + 1000" = rel32 of 1000-5=995, etc.
  CASE("jmp . + 1000",               x64_jmp_rel32(995));
  CASE("jmp . - 1000",               x64_jmp_rel32(-1005));
  CASE("call . + 1000",              x64_call_rel32(995));
  CASE("call . - 4",                 x64_call_rel32(-9));
  CASE("je . + 1000",                x64_jcc_rel32(x64_e, 994));
  CASE("jne . + 1000",               x64_jcc_rel32(x64_ne, 994));
  CASE("jl . - 300",                 x64_jcc_rel32(x64_l, -306));
  CASE("jge . + 300",                x64_jcc_rel32(x64_ge, 294));
  CASE("jo . + 300",                 x64_jcc_rel32(x64_o, 294));
  CASE("jbe . + 300",                x64_jcc_rel32(x64_be, 294));
  CASE("call rax",                   x64_call_r(x64_rax));
  CASE("call r11",                   x64_call_r(x64_r11));
  CASE("jmp rax",                    x64_jmp_r(x64_rax));
  CASE("jmp r11",                    x64_jmp_r(x64_r11));
  CASE("call qword ptr [rip+0x40]",  x64_call_rip(0x40));
  CASE("jmp qword ptr [rip-0x40]",   x64_jmp_rip(-0x40));

  // misc
  CASE("push rbp",                   x64_push_r(x64_rbp));
  CASE("push r12",                   x64_push_r(x64_r12));
  CASE("pop rbp",                    x64_pop_r(x64_rbp));
  CASE("pop r12",                    x64_pop_r(x64_r12));
  CASE("ret",                        x64_ret());
  CASE("leave",                      x64_leave());
  CASE("int3",                       x64_int3());
  CASE("ud2",                        x64_ud2());
  CASE("nop",                        x64_nop());

  fclose(sfile);
  fclose(wfile);

  // self-test the patch-field classifier against every patchable form
  int fails = 0;
  struct { const char* name; x64_insn i; int field_off, end_off; } pats[] = {
    { "jmp",      x64_jmp_rel32(0),       1, 5 },
    { "call",     x64_call_rel32(0),      1, 5 },
    { "jcc",      x64_jcc_rel32(x64_ne, 0), 2, 6 },
    { "call rip", x64_call_rip(0),        2, 6 },
    { "jmp rip",  x64_jmp_rip(0),         2, 6 },
    { "mov rip",  x64_mov_r_rip(x64_rax, 0), 3, 7 },
    { "mov rip9", x64_mov_r_rip(x64_r9, 0),  3, 7 },
    { "lea rip",  x64_lea_r_rip(x64_rcx, 0), 3, 7 },
    { "cmp rip",  x64_cmp_r_rip(x64_rdx, 0), 3, 7 },
  };
  for (unsigned k = 0; k < sizeof pats / sizeof pats[0]; k++) {
    unsigned char* end = 0;
    unsigned char* field = x64_patch_field(pats[k].i.b, &end);
    if (field != pats[k].i.b + pats[k].field_off || end != pats[k].i.b + pats[k].end_off) {
      fprintf(stderr, "patch classify failed: %s\n", pats[k].name);
      fails++;
    }
    if (end != pats[k].i.b + pats[k].i.len) {
      fprintf(stderr, "patch end != insn len: %s\n", pats[k].name);
      fails++;
    }
  }
  // and one non-patchable form
  {
    unsigned char* end = 0;
    x64_insn i = x64_add_r_r(x64_rax, x64_rcx);
    if (x64_patch_field(i.b, &end) != 0) {
      fprintf(stderr, "patch classify accepted add\n");
      fails++;
    }
  }
  if (fails) return 1;
  fprintf(stderr, "patch self-test ok\n");
  return 0;
}
