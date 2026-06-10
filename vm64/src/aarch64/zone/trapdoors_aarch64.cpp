# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "trapdoors.hh"
# pragma implementation "trapdoors_aarch64.hh"
# include "_trapdoors_aarch64.cpp.incl"

// No trapdoors needed on aarch64: call targets are absolute 8-byte words
// loaded from literal pools (see sendDesc_aarch64.hh), so there is no
// branch-span limit to work around.

Trapdoors::Trapdoors(pc_t, int32) {}

int32 Trapdoors::trapdoor_bytes() { return 0; }

pc_t Trapdoors::  SendMessage_stub_td(Location) { return first_inst_addr(   ::SendMessage_stub); }
pc_t Trapdoors::SendDIMessage_stub_td(Location) { return first_inst_addr( ::SendDIMessage_stub); }
pc_t Trapdoors::    Recompile_stub_td(Location) { return first_inst_addr(     ::Recompile_stub); }
pc_t Trapdoors::  DIRecompile_stub_td(Location) { return first_inst_addr(   ::DIRecompile_stub); }

pc_t Trapdoors::follow_trapdoors(pc_t target) { return target; } // no trapdoors

# endif // TARGET_ARCH == AARCH64_ARCH
