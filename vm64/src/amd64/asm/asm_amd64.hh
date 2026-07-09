# if defined(__x86_64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

# include "x64_encoding.hh"

// x86-64 assembler.
//
// Emits variable-length instruction byte strings into the BaseAssembler
// buffer via the verified encoders in x64_encoding.hh (see
// tools/test_x64_encoding.sh).  Sibling of asm_aarch64.hh; the Sun-era
// i386 assembler this file replaced lives on in vm/src/i386/asm/.
//
// Oop and address constants are materialized through a per-method literal
// pool exactly as on aarch64: loadOopLiteral/loadAddressLiteral emit a
// rip-relative MOV against a pool slot, and flushLiteralPool() lays the
// 8-byte words down (with their addrDescs) at the end of the method.  GC
// and IC patching then update plain data words -- never instruction bytes
// (see addrDesc_amd64.cpp).
//
// Branches are always emitted in their fixed-shape rel32 forms so a
// forward-referencing site can be backpatched in place (x64_patch_field
// classifies every patchable form).

class Assembler: public BaseAssembler {
 friend class Label; // for Backpatch

 public:

  Assembler(int32 instsSize, int32 locsSize, bool pr, bool isInstrs);

  // ---- emission core -------------------------------------------------
  void emit(const x64_insn& w);

  // Patch the rel32/disp32 field of the branch/rip-relative instruction
  // at destp to reach target.  Used by Label for forward-reference
  // resolution and by flushLiteralPool.
  void Backpatch(pc_t destp, pc_t target);

  // ---- loads/stores (disp is a byte offset; disp32 reaches everything) ----
  void load       (Location rt, Location base, fint disp);  // mov rt, [base+disp]
  void store      (Location base, fint disp, Location rt);  // mov [base+disp], rt
  void load32     (Location rt, Location base, fint disp);
  void store32    (Location base, fint disp, Location rt);
  void loadByte   (Location rt, Location base, fint disp);  // movzx rt, byte
  void storeByte  (Location base, fint disp, Location rt);
  // zero/immediate stores (no source register needed)
  void store_zero    (Location base, fint disp);
  void store_zero32  (Location base, fint disp);
  void storeByte_zero(Location base, fint disp);
  void add32_mem  (Location base, fint disp, fint imm);     // counter bump
  void lea        (Location rt, Location base, fint disp);

  // ---- constants ------------------------------------------------------
  void mov_imm(Location rd, smi value);          // minimal mov form, no addrDesc
  void loadOopLiteral(Location rd, oop p);       // pooled, records oop addrDesc
  void loadAddressLiteral(Location rd, void* a, OperandType t); // pooled VM address
  void flushLiteralPool();                       // call once, at end of method
  bool literalPoolIsEmpty() { return _nlits == 0; }
  void DataPtr(smi v);                           // raw 8-byte data word

  // ---- arithmetic/logical (two-address: rd = rd op src; flags set) ----
  void mov  (Location rd, Location rm);          // register move
  void add  (Location rd, Location rm);
  void add  (Location rd, fint imm);
  void sub  (Location rd, Location rm);
  void sub  (Location rd, fint imm);
  void cmp  (Location rn, fint imm);
  void cmp  (Location rn, Location rm);
  void andd (Location rd, smi bitmask);          // 'and' is a C++ keyword
  void andd (Location rd, Location rm);
  void orr  (Location rd, Location rm);
  void xorr (Location rd, Location rm);
  void tst  (Location rn, smi bitmask);
  void test (Location rn, Location rm);
  void neg  (Location rd);
  void shl  (Location rd, fint sh);
  void shr  (Location rd, fint sh);
  void sar  (Location rd, fint sh);
  void shl_cl(Location rd);                      // shift count in rcx
  void shr_cl(Location rd);
  void sar_cl(Location rd);
  void imul (Location rd, Location rm);
  void cqo  ();                                  // sign-extend rax into rdx:rax
  void idiv (Location rm);                       // rdx:rax / rm -> rax, rem rdx

  // ---- branches --------------------------------------------------------
  void jmp  (Label* L);
  void jcc  (x64_cond cc, Label* L);
  void call (Label* L);
  void call_reg(Location rn);
  void jmp_reg (Location rn);
  // call/jump through the 8-byte data word at dataWord (must lie in this
  // buffer; buffer-relative distances survive the copy into the zone)
  void call_mem(pc_t dataWord);
  void jmp_mem (pc_t dataWord);
  void ret  ();
  void leave();
  void push (Location rn);
  void pop  (Location rn);
  void int3 ();                                  // breakpoint (cf. brk)
  void ud2  ();                                  // guaranteed-undefined trap
  void nop  ();

  // align instsEnd to a multiple of `bytes` with nops
  void align(fint bytes);
  // pad so an instruction of length len emitted next ENDS on a multiple of
  // `bytes` (send sites: the return PC must be 8-aligned, see sendDesc)
  void align_end(fint bytes, fint len);

  // type-test counting instrumentation (see SICCountTypeTests)
  void startTypeTest(fint ncases, bool prologueCheck, bool immedOnly);
  void doOneTypeTest();
  void endTypeTest();

 private:
  // pending literal-pool entries (cf. asm_aarch64.hh)
  enum { MaxLits = 2048 };  // big typecase-heavy methods exceeded 256
  struct LitEntry {
    smi         value;
    OperandType type;
    pc_t        site;   // address of the referencing rip-relative mov
  };
  LitEntry _lits[MaxLits];
  fint     _nlits;

  void loadLiteral(Location rd, smi value, OperandType t);
};

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // defined(__x86_64__)
