# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

# include "a64_encoding.hh"

// aarch64 assembler.
//
// Emits fixed-width A64 words into the BaseAssembler buffer via the
// verified encoders in a64_encoding.hh (see tools/test_a64_encoding.sh).
//
// Oop and address constants are materialized through a per-method literal
// pool: loadOopLiteral/loadAddressLiteral emit a pc-relative LDR against a
// pool slot, and flushLiteralPool() lays the 8-byte words down (with their
// addrDescs) at the end of the method.  GC and IC patching then update
// plain data words -- never instruction bits (see addrDesc_aarch64.cpp).

class Assembler: public BaseAssembler {
 friend class Label; // for Backpatch

 public:

  Assembler(int32 instsSize, int32 locsSize, bool pr, bool isInstrs);

  // ---- emission core -------------------------------------------------
  void emit32(a64_inst w);

  // Patch the A64 branch/ldr-literal at destp to reach target.
  // Used by Label for forward-reference resolution.
  void Backpatch(pc_t destp, pc_t target);

  // ---- loads/stores (byte offsets; picks scaled or unscaled form) ----
  Location offset_scratch(Location rt, Location rn);  // for out-of-range offsets
  void ldr (Location rt, Location rn, fint byte_offset);
  void str (Location rt, Location rn, fint byte_offset);
  void ldrb(Location rt, Location rn, fint byte_offset);
  void strb(Location rt, Location rn, fint byte_offset);
  void stp (Location rt1, Location rt2, Location rn, fint byte_offset);
  void ldp (Location rt1, Location rt2, Location rn, fint byte_offset);
  // 32-bit forms (int32 counters and the like)
  void ldr32(Location rt, Location rn, fint byte_offset);
  void str32(Location rt, Location rn, fint byte_offset);
  void add32(Location rd, Location rn, fint imm);
  // zero stores (encode XZR as the source)
  void str_zero  (Location rn, fint byte_offset);
  void str_zero32(Location rn, fint byte_offset);
  void strb_zero (Location rn, fint byte_offset);

  // ---- constants ------------------------------------------------------
  void mov_imm(Location rd, smi value);          // movz/movk sequence, no addrDesc
  void loadOopLiteral(Location rd, oop p);       // pooled, records oop addrDesc
  void loadAddressLiteral(Location rd, void* a, OperandType t); // pooled VM address
  void flushLiteralPool();                       // call once, at end of method
  bool literalPoolIsEmpty() { return _nlits == 0; }
  void DataPtr(smi v);                           // raw 8-byte data word

  // ---- arithmetic/logical ---------------------------------------------
  void mov  (Location rd, Location rm);          // register move (handles SP)
  void add  (Location rd, Location rn, fint imm);
  void sub  (Location rd, Location rn, fint imm);
  void add  (Location rd, Location rn, Location rm);
  void sub  (Location rd, Location rn, Location rm);
  void adds (Location rd, Location rn, Location rm);
  void subs (Location rd, Location rn, Location rm);
  void cmp  (Location rn, fint imm);
  void cmp  (Location rn, Location rm);
  void andd (Location rd, Location rn, smi bitmask);   // 'and' is a C++ keyword
  void orr  (Location rd, Location rn, Location rm);
  void eor  (Location rd, Location rn, Location rm);
  void tst  (Location rn, smi bitmask);
  void lsl  (Location rd, Location rn, fint sh);
  void lsr  (Location rd, Location rn, fint sh);
  void asr  (Location rd, Location rn, fint sh);
  void mul  (Location rd, Location rn, Location rm);
  void sdiv (Location rd, Location rn, Location rm);

  // ---- branches --------------------------------------------------------
  void b    (Label* L);
  void b    (a64_cond cond, Label* L);
  void cbz  (Location rt, Label* L);
  void cbnz (Location rt, Label* L);
  void bl   (Label* L);
  void br   (Location rn);
  void blr  (Location rn);
  void ret  ();
  void brk  (fint imm16);
  void nop  ();
  void adr  (Location rd, Label* L);   // pc-relative address of a label

  // align instsEnd to a multiple of `bytes` with nops (e.g. before a send
  // site, whose return PC must be 8-aligned -- see sendDesc_aarch64.hh)
  void align(fint bytes);

  // type-test counting instrumentation (see SICCountTypeTests)
  void startTypeTest(fint ncases, bool prologueCheck, bool immedOnly);
  void doOneTypeTest();
  void endTypeTest();

 private:
  // pending literal-pool entries
  enum { MaxLits = 256 };
  struct LitEntry {
    smi         value;
    OperandType type;
    pc_t        site;   // address of the referencing ldr-literal instruction
  };
  LitEntry _lits[MaxLits];
  fint     _nlits;

  void emit_branch_to(Label* L, a64_inst (*enc0)(int), int rt_or_cond);
  void loadLiteral(Location rd, smi value, OperandType t);
};

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // defined(__aarch64__)
