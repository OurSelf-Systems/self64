# if TARGET_ARCH == AARCH64_ARCH
/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# include "_node_aarch64.cpp.incl"

# ifdef SIC_COMPILER

// aarch64 SIC code generation.
//
// Every gen() entry point below is a runtime-fatal stub: the compiler's
// machine-independent passes run, but emission is not yet implemented.
// These will be filled in against the AsmJit-backed Assembler.

static void unimplemented_gen(const char* who) {
  fatal1("aarch64 SIC code generation not yet implemented: %s", who);
}

  void PrologueNode::prePrologue()      { unimplemented_gen("PrologueNode::prePrologue"); }
  void PrologueNode::postPrologue()     { unimplemented_gen("PrologueNode::postPrologue"); }
  void PrologueNode::createStackFrame() { unimplemented_gen("PrologueNode::createStackFrame"); }

  void LoadIntNode::gen() {
    BasicNode::gen();
    unimplemented_gen("LoadIntNode::gen");
  }

  void AssignNode::genOop() { unimplemented_gen("AssignNode::genOop"); }

  void BranchNode::gen() {
    BasicNode::gen();
    unimplemented_gen("BranchNode::gen");
  }

  void TBranchNode::genCompare(bool ifEqual, Location l1, Location l2) {
    Unused(ifEqual); Unused(l1); Unused(l2);
    unimplemented_gen("TBranchNode::genCompare");
  }

  void TBranchNode::testTagsIfNecessary(bool ifInt, Location l1, Location l2) {
    Unused(ifInt); Unused(l1); Unused(l2);
    unimplemented_gen("TBranchNode::testTagsIfNecessary");
  }

  bool ArrayAtNode::genAccess(Location arr, Location index, Location dest) {
    Unused(arr); Unused(index); Unused(dest);
    unimplemented_gen("ArrayAtNode::genAccess");
    return false;
  }

  bool ByteArrayAtNode::genAccess(Location arr, Location index, Location dest) {
    Unused(arr); Unused(index); Unused(dest);
    unimplemented_gen("ByteArrayAtNode::genAccess");
    return false;
  }

  bool ArrayAtPutNode::genAccess(Location arr, Location index, Location dest) {
    Unused(arr); Unused(index); Unused(dest);
    unimplemented_gen("ArrayAtPutNode::genAccess");
    return false;
  }

  bool ByteArrayAtPutNode::genAccess(Location arr, Location index, Location dest) {
    Unused(arr); Unused(index); Unused(dest);
    unimplemented_gen("ByteArrayAtPutNode::genAccess");
    return false;
  }

  void ArithRCNode::gen() {
    BasicNode::gen();
    unimplemented_gen("ArithRCNode::gen");
  }

  Location arith_genHelper(PReg* sreg, PReg* oper, PReg* dest,
                           ArithOpCode op,
                           Location& t1, Location& t2, bool& reversed) {
    Unused(sreg); Unused(oper); Unused(dest); Unused(op);
    Unused(t1); Unused(t2); Unused(reversed);
    unimplemented_gen("arith_genHelper");
    return IllegalLocation;
  }

  void BlockZapNode::gen() {
    BasicNode::gen();
    unimplemented_gen("BlockZapNode::gen");
  }

  void RestartNode::gen() {
    BasicNode::gen();
    unimplemented_gen("RestartNode::gen");
  }

  void DeadEndNode::gen() {
    BasicNode::gen();
    unimplemented_gen("DeadEndNode::gen");
  }

  void BlockCreateNode::gen() {
    BasicNode::gen();
    unimplemented_gen("BlockCreateNode::gen");
  }

  void BlockCloneNode::genCall() { unimplemented_gen("BlockCloneNode::genCall"); }

  void IndexedBranchNode::gen() {
    BasicNode::gen();
    unimplemented_gen("IndexedBranchNode::gen");
  }

  void InterruptCheckNode::gen() {
    BasicNode::gen();
    unimplemented_gen("InterruptCheckNode::gen");
  }

  void MethodReturnNode::gen() {
    BasicNode::gen();
    unimplemented_gen("MethodReturnNode::gen");
  }

  void NonLocalReturnNode::gen() {
    BasicNode::gen();
    unimplemented_gen("NonLocalReturnNode::gen");
  }

  void StoreOffsetNode::gen() {
    BasicNode::gen();
    unimplemented_gen("StoreOffsetNode::gen");
  }

  void TypeTestNode::gen() {
    BasicNode::gen();
    unimplemented_gen("TypeTestNode::gen");
  }

  void UncommonNode::gen() {
    BasicNode::gen();
    unimplemented_gen("UncommonNode::gen");
  }

  bool AbstractArrayAtNode::canCopyPropagateFrom(PReg* d) {
    Unused(d);
    unimplemented_gen("AbstractArrayAtNode::canCopyPropagateFrom");
    return false;
  }

  void AbstractArrayAtNode::markAllocated(fint* use_count, fint* def_count) {
    Unused(use_count); Unused(def_count);
    unimplemented_gen("AbstractArrayAtNode::markAllocated");
  }

  Label* ByteArrayAtPutNode::testArg2() {
    unimplemented_gen("ByteArrayAtPutNode::testArg2");
    return NULL;
  }

  void AbstractArrayAtNode::gen() {
    BasicNode::gen();
    unimplemented_gen("AbstractArrayAtNode::gen");
  }

  void PrimNode::gen() {
    BasicNode::gen();
    unimplemented_gen("PrimNode::gen");
  }

  void SendNode::gen() {
    BasicNode::gen();
    unimplemented_gen("SendNode::gen");
  }

  void BasicNode::genBranch() {
    unimplemented_gen("BasicNode::genBranch");
  }

  void FlushNode::flushRegister(PReg* r) {
    Unused(r);
    unimplemented_gen("FlushNode::flushRegister");
  }

  bool TArithRRNode::isOpInlinable(ArithOpCode o) {
    Unused(o);
    // conservatively: nothing inlinable until the aarch64 backend exists
    return false;
  }

  bool TArithRRNode::canCopyPropagateFrom(PReg* d) {
    Unused(d);
    unimplemented_gen("TArithRRNode::canCopyPropagateFrom");
    return false;
  }

  void TArithRRNode::markAllocated(fint* use_count, fint* def_count) {
    Unused(use_count); Unused(def_count);
    unimplemented_gen("TArithRRNode::markAllocated");
  }

  void TArithRRNode::gen() {
    BasicNode::gen();
    unimplemented_gen("TArithRRNode::gen");
  }

# endif // SIC_COMPILER
# endif // TARGET_ARCH == AARCH64_ARCH
