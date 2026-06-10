# if defined(__aarch64__)
/* Copyright 2024-2026 AUTHORS.
   See the LICENSE file for license information. */

/* aarch64 register and Location definitions.

   Self's compiled-code calling convention on aarch64 deliberately mirrors
   the i386 scheme for bring-up: the receiver and all arguments are passed
   on the stack (NumArgRegisters == 0) and ReceiverReg & friends are
   synthetic Locations mapped to frame slots by frame_format_aarch64.
   Register-based argument passing is a later optimization.

   Register roles:
     x0          ResultReg / CResultReg / NLRResultReg (matches C ABI)
     x1 - x3     NLR home / homeID / temp registers
     x4 - x7     TempRegs: BB-local temps for the SIC register allocator
     x8          unused (C ABI indirect-result register)
     x9          Temp1 (reserved for the code generator)
     x10         Temp2 (reserved for the code generator)
     x11, x12    Temp3, Temp4 (SIC scratch, reserved via markAllocated)
     x13 - x15   unallocated scratch (future allocator use)
     x16, x17    reserved: send-site call sequences (ldr/blr) and veneers
     x18         RESERVED platform register (macOS) -- never touched
     x19, x20    PerformSelectorLoc / PerformDelegateeLoc (callee-saved,
                 survive the C lookup call in the perform path)
     x21 - x28   callee-saved, unallocated for now
     x29         FrameReg (fp) -- frames are walked via the fp chain
     x30         lr
     sp          stack pointer (16-byte alignment maintained at calls)

   Volatility is conservative, as on i386: AllTrashedMask covers every
   allocatable register, i.e. nothing is assumed to survive a Self call. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif

  const fint NumRegisters          = 32;  // x0..x30 + sp
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
    // change RegisterNames[] in regs_aarch64.cpp if you change this enum!
    x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,
    x8,  x9,  x10, x11, x12, x13, x14, x15,
    x16, x17, x18, x19, x20, x21, x22, x23,
    x24, x25, x26, x27, x28, x29, x30,
    sp,                         // hardware encoding 31
    no_reg,

    fp = x29, lr = x30,
    SP = sp, NoReg = no_reg,

    UnAllocated = -10, // unused or not yet allocated, needs to be unique

    PerformSelectorLoc  = x19,
    PerformDelegateeLoc = x20,

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

    ResultReg  = x0,
    CResultReg = ResultReg,

    Temp1 = x9,   // reserved for the code generator
    Temp2 = x10,  // reserved for the code generator
#   ifdef SIC_COMPILER
    // additional temps, reserved through markAllocated (see i386 comments)
    Temp3 = x11,
    Temp4 = x12,
#   endif

    FrameReg = x29,

    // for di lookups: these must be distinct and not arg regs
    DICountReg       = Temp1,
    DIInlineCacheReg = Temp2,

    // while doing an NLR, store important "return values" in registers.
    // Home and Result must not coincide with Temp1 because of zapBlock.
    NLRResultReg = x0,
    NLRHomeReg   = x1,
    NLRHomeIDReg = x2,
    NLRTempReg   = x3,

    ByteMapBaseReg = no_reg,

      LowestNonVolReg =  0,   // like i386: nothing survives a Self call
     HighestNonVolReg = -1,
 LowestLocalNonVolReg =  0,
 };


inline bool  is_IArgLocation(Location r) { return  IArgLocations <= r  &&  r <   EndIArgLocations; }
inline bool  is_LArgLocation(Location r) { return  LArgLocations <= r  &&  r <   EndLArgLocations; }
inline bool   is_ArgLocation(Location r) { return   ArgLocations <= r  &&  r <    EndArgLocations; }
inline bool is_StackLocation(Location r) { return StackLocations <= r  &&  r <  EndStackLocations; }


inline bool isRegister(Location r) { return  x0 <= r  &&  r <= sp; }
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
  // (bits x0..x30; sp excluded)
  const RegisterString AllTrashedMask = RegisterString(0x7FFFFFFF);

  const fint NumCalleeSavedRegs = 0;
  extern Location CalleeSavedRegs[];

  inline bool isInitializedInFillValues(Location loc) {
    Unused(loc);
    fatal("unimplemented for aarch64");
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

# endif // defined(__aarch64__)
