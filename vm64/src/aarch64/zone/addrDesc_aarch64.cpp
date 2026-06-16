# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "addrDesc_aarch64.hh"

# include "_addrDesc_aarch64.cpp.incl"


# if  defined(FAST_COMPILER) || defined(SIC_COMPILER)

// On aarch64, relocatable referents (oops, send/DI targets, primitive
// addresses) are absolute 8-byte words in literal pools inside the code
// stream; pc-relative references are never recorded as addrDescs.  This
// keeps GC updates and call-site patching simple atomic stores.

bool addrDesc::isShiftNeededAfterMovingMe(OopNCode* m) {
  Unused(m);
  return isRelative();
}


pc_t addrDesc::instr_referent(OopNCode* m) {
  pc_t* dispp = addr(m);
  assert(m->contains(dispp), "not in this nmethod");
  assert(!isRelative(), "aarch64 addrDescs hold absolute words");
  return *dispp;
}


void addrDesc::set_instr_referent(OopNCode* m, void* newVal) {
  pc_t* dispp = addr(m);
  assert(m->contains(dispp), "not in this nmethod");
  assert(!isRelative(), "aarch64 addrDescs hold absolute words");
  *dispp = pc_t(newVal);
}


void addrDesc::relocateTarget(OopNCode* m, int32 delta) {
  pc_t* dispp = addr(m);
  assert(m->contains(dispp), "not in this nmethod");
  Unused(delta);
  assert(!isRelative(), "aarch64 addrDescs hold absolute words");
  // absolute words need no adjustment when this nmethod moves
}


bool addrDesc::verify(nmethod* m) {
  bool flag = true;
  if (offset() >= m->instsLen() + m->scopes->length()) {
    error1("bad offset in addrDesc at %#lx", (long)this);
    flag = false;
  }
  if (isSendDesc()) {
    flag = asSendDesc(m)->verify() && flag;
  }
  else if (isDIDesc()) {
    flag = asDIDesc(m)->dependency()->verify_list_integrity() && flag;
  }
  else if (isPrimitive())
    ;
  else
    flag = oop(referent(m))->verify_oop() && flag;

  return flag;
}


// not inlined to reduce .h dependencies
sendDesc* addrDesc::asSendDesc(OopNCode* m) {
  assert(isSendDesc(), "not a sendDesc location");
  return sendDesc::sendDesc_from_addrDesc_addr(addr(m));
}

sendDesc* addrDesc::asPrimitiveSendDesc(OopNCode* m) {
  // note that it's not really an inline cache, just a primitive call
  assert(isPrimitive(), "not a primitive location");
  return sendDesc::sendDesc_from_addrDesc_addr(addr(m));
}

DIDesc* addrDesc::asDIDesc(nmethod* m) {
  assert(isDIDesc(), "not a diDesc location");
  return DIDesc::DIDesc_from_addrDesc_addr(addr(m));
}


# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
# endif // TARGET_ARCH == AARCH64_ARCH
