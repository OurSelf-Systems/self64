# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif

// aarch64 instruction predicates.
// aarch64 instructions are fixed-width 32-bit words.

// uncommon-trap bookkeeping (see uncommonBranch_aarch64.cpp)
const int UncommonRestartBit = 1 << 31;


// Any smi can be materialized via a literal-pool load, so immediates are
// never range-limited (cf. SPARC's 13-bit simm).
inline bool isImmediate(smiOop value) {
  Unused(value);
  return true;
}


inline bool isCall(int32* instp) {
  // BL: top 6 bits == 100101
  return (((uint32_t)instp[-1]) & 0xFC000000u) == 0x94000000u;
}


inline bool isUnimp(int32* instp) {
  // BRK #imm16: 1101 0100 001x xxxx xxxx xxxx xxx0 0000
  return (((uint32_t)instp[0]) & 0xFFE0001Fu) == 0xD4200000u;
}

# endif // defined(__aarch64__)
