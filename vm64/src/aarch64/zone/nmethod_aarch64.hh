# if defined(__aarch64__)

/* Copyright 1992-2026 AUTHORS.
   See the LICENSE file for license information. */


// included into midst of class nmethod

private:
  fint _number_of_memory_locals;

public:
  fint number_of_memory_locals()                { return _number_of_memory_locals; }
  fint incoming_arg_register_count()            { return min(NumIArgRegisters, _incoming_arg_count); }
# endif // defined(__aarch64__)
