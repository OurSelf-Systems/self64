# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif

// included in sic.hh


public:
  fint   max_no_of_outgoing_args_and_rcvr();
  fint   number_of_memory_locals();

public:
  Label* nlrLabel;     // used for caching NLR return

private:

# endif // defined(__aarch64__)
