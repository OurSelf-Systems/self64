# if defined(__aarch64__)
/* Copyright 2024-2026 AUTHORS.
   See the LICENSE file for license information. */


 public:
  int* fp_addr();
  frame* fp();   // full-width read of the interrupted frame pointer
# endif // defined(__aarch64__)
