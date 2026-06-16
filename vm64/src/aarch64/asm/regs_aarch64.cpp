# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 2024-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_regs_aarch64.cpp.incl"

// change Location enum in regs_aarch64.hh if you change this!
const char* RegisterNames[] = {
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
    "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "fp",  "lr",
    "sp",
    "",

    "*UnAllocated*"
};

// aarch64 has no i386-style sub-register names; the w forms are provided
// so generic printing code can index these arrays.
const char* ByteRegisterNames[] = {
    "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
    "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
    "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
    "w24", "w25", "w26", "w27", "w28", "w29", "w30",
    "wsp",
    "",

    "*UnAllocated*"
};
const char* ShortRegisterNames[] = {
    "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
    "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
    "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
    "w24", "w25", "w26", "w27", "w28", "w29", "w30",
    "wsp",
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
  assert(mask == 0, "unused for aarch64");
}


# if defined(SIC_COMPILER)

  Location TempRegs[] = { x4, x5, x6, x7 };

# define X(arg) -99999999     /* to make the following table look nicer */
  fint RegToTempNo[/* indexed by Location */] = {
    X("x0"),  X("x1"),  X("x2"),  X("x3"),  0, 1, 2, 3,
    X("x8"),  X("Temp1"), X("Temp2"), X("x11"), X("x12"),
    X("x13"), X("x14"), X("x15"),
    X("x16"), X("x17"), X("x18"), X("x19"), X("x20"), X("x21"), X("x22"),
    X("x23"), X("x24"), X("x25"), X("x26"), X("x27"), X("x28"),
    X("fp"),  X("lr"),  X("sp")
  };
# undef X

  Location CalleeSavedRegs[] = {
  };

  void regs_amd64_init() {
    // The SIC uses Temp1 and Temp2 during code generation, that's why they
    // aren't listed as general temp regs above.
    assert(Temp1 == x9 && Temp2 == x10, "change this");
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

# endif // TARGET_ARCH == AARCH64_ARCH
