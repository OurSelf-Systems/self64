# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif

// included in genHelper.hh

public:
  void moveToExactlyThisReg(PReg* pr, Location reg);

private:

# endif // defined(__aarch64__)
