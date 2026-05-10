// diag_toggles.cpp — startup banner showing which DIAG_* topics are on.
//   -- claude & dmu May 2026

# include "_diag_toggles.cpp.incl"

void print_active_diag_toggles_at_startup() {
  // fprintf(stderr) avoids any dependency on lprintf being ready.
  bool any = false;
# define SHOW(name) do {                                                 \
    if (name) {                                                          \
      if (!any) { fprintf(stderr, "[DIAG] active toggles:"); any = true; }\
      fprintf(stderr, " " #name);                                        \
    }                                                                    \
  } while (0)
  SHOW(DIAG_TRACK_BLOCKS_AND_VFRAMES_ACROSS_INTERPRETERS);
  SHOW(DIAG_SCAVENGED_INTERPRETER_STACK_RANGES);
  SHOW(DIAG_ACTIVATION_DUMP);
  SHOW(DIAG_ARGS_WATCH);
  SHOW(DIAG_ZAP_FREED_INTERPRETERS);
  SHOW(DIAG_SHELL_TRACE);
# undef SHOW
  if (any) fprintf(stderr, "\n");
  else     fprintf(stderr, "[DIAG] no toggles active\n");
}

namespace {
  struct DiagTogglesStartupBanner {
    DiagTogglesStartupBanner() { print_active_diag_toggles_at_startup(); }
  };
  DiagTogglesStartupBanner g_diag_toggles_startup_banner;
}
