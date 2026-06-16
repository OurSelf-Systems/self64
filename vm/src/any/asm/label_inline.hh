/* Sun-$Revision: 30.10 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


# if  defined(FAST_COMPILER) || defined(SIC_COMPILER)


#if GENERATE_DEBUGGING_AIDS

// theAssembler can be NULL when a stack-allocated Label outlives the
// assembler it was created under (e.g. generate_runtime_stubs_into restores
// theAssembler = NULL before its locals destruct at startup).  -- rca 6/26
inline void Label::  trackLabel()  { if (theAssembler) theAssembler->  trackLabel(this); }
inline void Label::untrackLabel()  { if (theAssembler) theAssembler->untrackLabel(this); }

#endif // GENERATE_DEBUGGING_AIDS


# endif // defined(FAST_COMPILER) || defined(SIC_COMPILER)
