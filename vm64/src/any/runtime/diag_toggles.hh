// diag_toggles.hh — per-topic compile-time switches for DIAGNOSTIC code.
// Flip a topic to 0 to compile out all sites tagged `#if DIAG_<topic>`.
// Every gated site also carries a `/* DIAGNOSTIC */` comment so
// `grep DIAGNOSTIC` still finds every region.
//   -- claude & dmu May 2026

#ifndef DIAG_TOGGLES_HH
#define DIAG_TOGGLES_HH


// Prints which DIAG_* topics are active. Called once at process start
// via a static-init struct in diag_toggles.cpp, so it always runs even
// when nothing else in the program references it.
void print_active_diag_toggles_at_startup();

#endif // DIAG_TOGGLES_HH
