# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


# if defined(FAST_COMPILER) || defined(SIC_COMPILER)

// aarch64 assembler.
//
// This class currently declares only the surface the machine-independent
// compiler code needs to compile; the instruction-emission API is added
// as the aarch64 code generator (backed by AsmJit) is implemented.
// Until then, emission entry points are runtime-fatal stubs.

class Assembler: public BaseAssembler {
 friend class Label; // for Backpatch

 public:

  Assembler(int32 instsSize, int32 locsSize, bool pr, bool isInstrs);

  // Patch the (4-byte aarch64 branch) instruction at destp to reach target.
  // Used by Label for forward-reference resolution.
  void Backpatch(pc_t destp, pc_t target);

  // type-test counting instrumentation (see SICCountTypeTests)
  void startTypeTest(fint ncases, bool prologueCheck, bool immedOnly);
  void doOneTypeTest();
  void endTypeTest();
};

# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // defined(__aarch64__)
