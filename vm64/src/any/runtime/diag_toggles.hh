// diag_toggles.hh — per-topic compile-time switches for DIAGNOSTIC code.
// Flip a topic to 0 to compile out all sites tagged `#if DIAG_<topic>`.
// Every gated site also carries a `/* DIAGNOSTIC */` comment so
// `grep DIAGNOSTIC` still finds every region.
//   -- claude & dmu May 2026

#ifndef DIAG_TOGGLES_HH
#define DIAG_TOGGLES_HH

// Lifecycle/event ring buffer in interpreter.cpp + atexit/fatal dumpers.
// Includes: g_interp_diag_buf, diag_interp_event, diag_dump_interp_now,
//           InterpDiagAtexitInstaller, lprint_fatal dump, CB_BAD/CB_WRITE,
//           POISON-result detector, vframe.cpp PROBE events.
#define DIAG_TRACK_BLOCKS_AND_VFRAMES_ACROSS_INTERPRETERS       1

// Per-activation pc ring + pre_send_*/saved_selector_* capture +
// print_activation_diag + capture_pre_send_diag + the
// get_argument_count fatal dump.
#define DIAG_ACTIVATION_DUMP   1

// args[] good->bad watcher inside interpret_method.
// Depends on DIAG_SCAV_RANGES (uses g_scav_round, g_scav_range_count,
// diag_scav_ranges_contains, diag_scav_frames_contains): if you set this to 1,
// keep DIAG_SCAV_RANGES=1 too.
#define DIAG_ARGS_WATCH        1


// Per-scavenge range + frame recorders (stack.cpp, universe.more.cpp,
// interpreter.hh InterpreterIterator).
#define DIAG_SCAVENGED_INTERPRETER_STACK_RANGES       (0 || DIAG_ARGS_WATCH)


// Note: the `preserved` wraps around interpret()'s parameters are a real
// GC-root fix and stay unconditional even when this is 0.
// Producer of DIAG_INTERPRETER_ZAP_VALUE. Consumers (DIAG_TRACK_BLOCKS_AND_VFRAMES_ACROSS_INTERPRETERS ZAP-result
// detector, DIAG_SHELL_TRACE map=ZAP-ALREADY check) only fire when
// this is 1; with this 0 the detectors compile but never trigger.
#define DIAG_ZAP_FREED_INTERPRETERS  1

#if DIAG_ZAP_FREED_INTERPRETERS
// 8-byte zap pattern written to oop slots when DIAG_ZAP_FREED_INTERPRETERS=1.
// Must equal byte-0xde repeated across the full oop width so a memset-style
// byte fill of the surrounding struct is also detected as zapped.
//   -- claude & dmu May 2026
#define DIAG_INTERPRETER_ZAP_VALUE  0xdedededededededeULL
#endif

// shell.cpp evalExpressions map-print + snapshotAction postRead trace.
// (The `pres_method`/`pres_rcv` wraps are substantive, kept unconditional.)
#define DIAG_SHELL_TRACE       1

// Prints which DIAG_* topics are active. Called once at process start
// via a static-init struct in diag_toggles.cpp, so it always runs even
// when nothing else in the program references it.
void print_active_diag_toggles_at_startup();

#endif // DIAG_TOGGLES_HH
