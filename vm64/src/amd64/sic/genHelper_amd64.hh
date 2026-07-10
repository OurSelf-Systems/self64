# if defined(__x86_64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif

// included in genHelper.hh

public:
  void moveToExactlyThisReg(PReg* pr, Location reg);

private:
  void verifyOneImmediateParent(assignableSlotLink* l,
                                Location parentOopReg,
                                Location scratchReg,
                                fint count);
  void verifyConstrainedOopOfParent(oop targetOop,
                                    Location parentOopReg,
                                    Label* ok);
  void verifyMapOfParent(Map* targetMap,
                         Location parentOopReg,
                         Location regForMap,
                         Label* ok);

# endif // defined(__x86_64__)
