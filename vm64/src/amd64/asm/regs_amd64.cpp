# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "regs_amd64.hh"

# include "_regs_amd64.cpp.incl"

// change Location enum in regs_amd64.hh if you change this!
const char* RegisterNames[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    "",

    "*UnAllocated*"
};
const char* ByteRegisterNames[] = {
    "al",  "cl",  "dl",  "bl",  "spl", "bpl", "sil", "dil",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
    "",

    "*UnAllocated*"
};
const char* ShortRegisterNames[] = {
    "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",
    "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
    "",

    "*UnAllocated*"
};

const char** RegisterNamesBySize[] =
  { ByteRegisterNames, ShortRegisterNames, RegisterNames };


const char *locationName(Location l) {
  const char* c;
  int num;

       if ( is_IArgLocation(l)) {   c = "I";  num = index_for_IArgLocation(l); }
  else if ( is_LArgLocation(l)) {   c = "L";  num = index_for_LArgLocation(l); }
  else if ( is_ArgLocation(l))  {   c = "";   num = index_for_ArgLocation(l); }
  else if ( is_StackLocation(l)){   c = "S";  num = index_for_StackLocation(l); }
  else if ( l == IReceiverReg ) {   c = "I";  num = -1; }
  else if ( l == LReceiverReg ) {   c = "L";  num = -1; }
  else if ( l == ReceiverReg )  {   c = "R";  num = -1; }
  else {
    assert(isRegister(l), "");
    return RegisterNames[l];
  }
  char* s = new char[30];
  sprintf(s, "%s%ld", c, long(num));
  return s;
}


void printMask(RegisterString mask) {
  assert(mask == 0, "unused for amd64");
}


# if defined(SIC_COMPILER)

  Location TempRegs[] = { rdi, rbx, r8, r9 };

# define X(arg) -99999999     /* to make the following table look nicer */
  fint RegToTempNo[/* indexed by Location */] = {
    X("rax"), X("rcx"), X("rdx"), 1, X("rsp"), X("rbp"), X("rsi"), 0,
    2, 3, X("Temp1"), X("Temp2"), X("r12"), X("r13"), X("Temp3"), X("Temp4")
  };
# undef X

  Location CalleeSavedRegs[] = {
  };

  void regs_amd64_init() {
    // The SIC uses Temp1 and Temp2 during code generation, that's why they
    // aren't listed as general temp regs above.
    assert(Temp1 == r10 && Temp2 == r11, "change this");
    for (fint i = 0; i < NumTempRegs; i++) {
      assert(TempRegs[i] != Temp1 && TempRegs[i] != Temp2,
             "Temp1 and Temp2 are reserved for the code generator");
      assert(RegToTempNo[TempRegs[i]] == i, "wrong RegToTempNo entry");
    }
  }

# else

  Location TempRegs[] = {};
  fint RegToTempNo[] = {};
  Location CalleeSavedRegs[] = {};

  void regs_amd64_init() {
    // No JIT register setup without SIC_COMPILER
  }

# endif // defined(SIC_COMPILER)

# endif // TARGET_ARCH == X86_64_ARCH
