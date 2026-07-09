# if TARGET_ARCH == X86_64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "trapdoors.hh"
# include "_trapdoors_amd64.cpp.incl"

// amd64 has no branch-span problem (call targets are absolute words in
// literal pools), but the trapdoor area at the zone base is the home of
// the generated runtime stubs: EnterSelf and SendMessage_stub live here,
// outside both the nmethod and PIC heaps, mirroring i386's text-resident
// stubs for the zone-taxonomy checks in sendDesc::pic()/countStub().
// (Structure follows trapdoors_aarch64.cpp.)

# ifdef SIC_COMPILER
extern int32 generate_runtime_stubs_into(char* dst, int32 avail);  // stubs_amd64.cpp
# endif

static int32 generated_stub_bytes = 0;

Trapdoors::Trapdoors(pc_t start_of_code, int32 code_size) {
# ifdef SIC_COMPILER
  generated_stub_bytes = generate_runtime_stubs_into(start_of_code, code_size);
# else
  Unused(start_of_code); Unused(code_size);
# endif
}

int32 Trapdoors::trapdoor_bytes() { return generated_stub_bytes; }

extern char* amd64_SendMessage_stub();    // stubs_amd64.cpp
extern char* amd64_SendDIMessage_stub();  // stubs_amd64.cpp

pc_t Trapdoors::  SendMessage_stub_td(Location) { return amd64_SendMessage_stub(); }
pc_t Trapdoors::SendDIMessage_stub_td(Location) { return amd64_SendDIMessage_stub(); }
pc_t Trapdoors::    Recompile_stub_td(Location) { return first_inst_addr(     ::Recompile_stub); }
pc_t Trapdoors::  DIRecompile_stub_td(Location) { return first_inst_addr(   ::DIRecompile_stub); }

pc_t Trapdoors::follow_trapdoors(pc_t target) { return target; } // no trapdoors

# endif // TARGET_ARCH == X86_64_ARCH
