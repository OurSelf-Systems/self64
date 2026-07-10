# if defined(__x86_64__)
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

/* x86-64 register and Location definitions.

   Self's compiled-code calling convention on amd64 deliberately mirrors
   the i386/aarch64 scheme for bring-up: the receiver and all arguments are
   passed on the stack (NumArgRegisters == 0) and ReceiverReg & friends are
   synthetic Locations mapped to frame slots by frame_format_amd64.
   Register-based argument passing is a later optimization.

   Enum values are ModRM hardware numbers (bit 3 = REX.R/B), so a Location
   can be handed straight to the x64_encoding.hh encoders.

   Register roles (cf. regs_aarch64.hh):
     rax         ResultReg / CResultReg / NLRResultReg (matches C ABI)
     rcx         NLRHomeIDReg (also the shift-count register; NLR regs are
                 only live while an NLR unwind is in flight)
     rdx         NLRHomeReg
     rbx         TempRegs[1]
     rsp         stack pointer (16-byte alignment maintained at calls)
     rbp         FrameReg -- frames are walked via the rbp chain
     rsi         NLRTempReg
     rdi         TempRegs[0]
     r8,  r9     TempRegs[2], TempRegs[3]: BB-local temps for the SIC
                 register allocator
     r10, r11    Temp1, Temp2 (reserved for the code generator)
     r12, r13    PerformSelectorLoc / PerformDelegateeLoc (callee-saved,
                 survive the C lookup call in the perform path)
     r14, r15    Temp3, Temp4 (SIC scratch, reserved via markAllocated)

   No registers are reserved for the assembler itself: disp32 addressing
   and [rip+disp32] sends need no scratch or veneer registers (the aarch64
   assembler reserves x16/x17 for both).

   rbx and r12-r15 are callee-saved in the C ABI but the JIT clobbers them
   freely; the EnterSelf glue must save/restore all five around any entry
   into Self code (the i386 EnterSelf saved ebx/esi/edi the same way).

   Volatility is conservative, as on i386: AllTrashedMask covers every
   allocatable register, i.e. nothing is assumed to survive a Self call. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif

  const fint NumRegisters          = 16;
  const fint NumArgRegisters       = 0;
  const fint NumRcvrAndArgRegisters = 0;
  const fint NumIArgRegisters      = 0;
  const fint NumRegistersInMask    = 0;

  extern const char* RegisterNames[];
  extern const char* ByteRegisterNames[];
  extern const char* ShortRegisterNames[];

  enum RegSize { byte_reg, short_reg, long_reg };

  extern const char** RegisterNamesBySize[];

  enum Location {
    // change RegisterNames[] in regs_amd64.cpp if you change this enum!
    rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi,
    r8,  r9,  r10, r11, r12, r13, r14, r15,
    no_reg,

    // legacy aliases: the shared frame code's x86 paths predate the
    // 64-bit renaming (e.g. frame_format_amd64.cpp)
    eax = rax, ecx = rcx, edx = rdx, ebx = rbx,
    esp = rsp, ebp = rbp, esi = rsi, edi = rdi,
    sp = rsp,

    SP = rsp, NoReg = no_reg,

    UnAllocated = -10, // unused or not yet allocated, needs to be unique

    PerformSelectorLoc  = r12,
    PerformDelegateeLoc = r13,

    StackLocations = 100, // used to represent expr stack locations and locals
    EndStackLocations = 9999,

    ReceiverReg     = 10000, // not a reg at all! (stack-passing convention)
    ArgLocations    = 10001, // outgoing args on the stack
    EndArgLocations = 19999,

    // where post-frame-creation incoming args are stored:
    IReceiverReg       = 20000,
    IArgLocations      = 20001, // incoming args on stack (in caller's frame)
    EndIArgLocations   = 29999,

    // for Leaf methods
    LReceiverReg       = 30000,
    LArgLocations      = 30001,
    EndLArgLocations   = 39999,

    IllegalLocation = -1,

    ResultReg  = rax,
    CResultReg = ResultReg,

    Temp1 = r10,  // reserved for the code generator
    Temp2 = r11,  // reserved for the code generator
#   ifdef SIC_COMPILER
    // additional temps, reserved through markAllocated (see
    // AbstractArrayAtNode::markAllocated and the i386 comments)
    Temp3 = r14,
    Temp4 = r15,
#   endif

    FrameReg = rbp,

    // for di lookups: these must be distinct and not arg regs
    DICountReg       = Temp1,
    DIInlineCacheReg = Temp2,

    // while doing an NLR, store important "return values" in registers.
    // Home and Result must not coincide with Temp1 because of zapBlock.
    NLRResultReg = rax,
    NLRHomeReg   = rdx,
    NLRHomeIDReg = rcx,
    NLRTempReg   = rsi,

    ByteMapBaseReg = no_reg,  // doesn't seem worthwhile on Intel

      LowestNonVolReg =  0,   // like i386: nothing survives a Self call
     HighestNonVolReg = -1,
 LowestLocalNonVolReg =  0,
 };


inline bool  is_IArgLocation(Location r) { return  IArgLocations <= r  &&  r <   EndIArgLocations; }
inline bool  is_LArgLocation(Location r) { return  LArgLocations <= r  &&  r <   EndLArgLocations; }
inline bool   is_ArgLocation(Location r) { return   ArgLocations <= r  &&  r <    EndArgLocations; }
inline bool is_StackLocation(Location r) { return StackLocations <= r  &&  r <  EndStackLocations; }


inline bool isRegister(Location r) { return  rax <= r  &&  r <= r15; }
inline bool isStackRegister(Location r) { return is_StackLocation(r); }


// 0 = first arg, -1 = rcvr
inline Location  IArgLocation(fint i)  {  return Location(  IArgLocations + i); }
inline Location  LArgLocation(fint i)  {  return Location(  LArgLocations + i); }
inline Location   ArgLocation(fint i)  {  return Location(   ArgLocations + i); }
inline Location StackLocation_for_index(fint i)  {  return Location( StackLocations + i); }

inline fint index_for_IArgLocation( Location x) { return x - IArgLocations; }
inline fint index_for_LArgLocation( Location x) { return x - LArgLocations; }
inline fint index_for_ArgLocation(  Location x) { return x - ArgLocations; }
inline fint index_for_StackLocation(Location x) { return x - StackLocations; }


const RegisterString GloballyAllocatedMask = 0;
const RegisterString LocalMask = 0;


# ifdef SIC_COMPILER
  const fint NumTempRegs = 4; // regs that can be used within BB's
                              // but do not survive calls
  extern Location TempRegs[];
  extern fint RegToTempNo[]; // inverse of TempRegs[]

  inline bool isTempReg(Location r) {
    return isRegister(r) && RegToTempNo[r] >= 0;
  }

  // conservative: every allocatable register is trashed by a call
  // (all 16 minus rsp; rbp is included the way aarch64 includes x29)
  const RegisterString AllTrashedMask = RegisterString(0xFFEF);

  const fint NumCalleeSavedRegs = 0;
  extern Location CalleeSavedRegs[];

  inline bool isInitializedInFillValues(Location loc) {
    Unused(loc);
    fatal("unimplemented for amd64");
    return false;
  }

  inline bool isArgRegister(Location) { return false; }

# else

  const fint NumTempRegs        = 0;
  const fint NumCalleeSavedRegs = 0;
  const RegisterString AllTrashedMask = 0;

  extern Location TempRegs[];
  extern fint     RegToTempNo[];
  extern Location CalleeSavedRegs[];

  inline bool isTempReg(Location) { return false; }
  inline bool isInitializedInFillValues(Location) { return false; }
  inline bool isArgRegister(Location) { return false; }

# endif

# endif // defined(__x86_64__)
