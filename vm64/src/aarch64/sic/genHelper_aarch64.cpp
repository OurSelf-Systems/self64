# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_genHelper_aarch64.cpp.incl"

# ifdef SIC_COMPILER

// aarch64 SICGenHelper.
//
// Runtime-fatal stubs until the AsmJit-backed code generator lands;
// see node_aarch64.cpp.

static void unimplemented_helper(const char* who) {
  fatal1("aarch64 SICGenHelper not yet implemented: %s", who);
}

void SICGenHelper::smiOop_prologue(char* miss) {
  Unused(miss);
  unimplemented_helper("smiOop_prologue");
}

void SICGenHelper::memOop_prologue(mapOop receiverMapOop, char* miss) {
  Unused(receiverMapOop); Unused(miss);
  unimplemented_helper("memOop_prologue");
}

void SICGenHelper::genCountCode(int32* counter) {
  Unused(counter);
  unimplemented_helper("genCountCode");
}

Location SICGenHelper::loadImmediateOop(ConstPReg* p, Location dest, bool mustMove) {
  Unused(p); Unused(dest); Unused(mustMove);
  unimplemented_helper("loadImmediateOop(ConstPReg*)");
  return IllegalLocation;
}

void SICGenHelper::loadImmediateOop(oop p, Location dest, bool isInt) {
  Unused(p); Unused(dest); Unused(isInt);
  unimplemented_helper("loadImmediateOop(oop)");
}

void SICGenHelper::moveRegToReg(Location srcReg, Location destReg) {
  Unused(srcReg); Unused(destReg);
  unimplemented_helper("moveRegToReg");
}

void SICGenHelper::setToZeroA(void* addr, Location tempReg) {
  Unused(addr); Unused(tempReg);
  unimplemented_helper("setToZeroA");
}

fint SICGenHelper::verifyParents(objectLookupTarget* target, Location t, fint count) {
  Unused(target); Unused(t); Unused(count);
  unimplemented_helper("verifyParents");
  return 0;
}

fint SICGenHelper::spOffset(Location l) {
  Unused(l);
  unimplemented_helper("spOffset");
  return 0;
}

fint SICGenHelper::spOffset(Location l, nmethod* nm) {
  Unused(l); Unused(nm);
  unimplemented_helper("spOffset(nmethod)");
  return 0;
}

void SICGenHelper::floatOop_prologue(char* miss) {
  Unused(miss);
  unimplemented_helper("floatOop_prologue");
}

void SICGenHelper::checkOop(Label& general, oop what, Location reg) {
  Unused(general); Unused(what); Unused(reg);
  unimplemented_helper("checkOop");
}

void SICGenHelper::load(Location src, fint srcOffset, Location dest) {
  Unused(src); Unused(srcOffset); Unused(dest);
  unimplemented_helper("load");
}

void SICGenHelper::store(Location src, fint dstOffset, Location dest) {
  Unused(src); Unused(dstOffset); Unused(dest);
  unimplemented_helper("store");
}

void SICGenHelper::setToZero(Location dest) {
  Unused(dest);
  unimplemented_helper("setToZero");
}

# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
