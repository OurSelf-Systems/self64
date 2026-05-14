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
 //  SHOW(DIAG_wHATEVER);
# undef SHOW
  if (any) fprintf(stderr, "\n");
 // else     fprintf(stderr, "[DIAG] no toggles active\n");
}

namespace {
  struct DiagTogglesStartupBanner {
    DiagTogglesStartupBanner() { print_active_diag_toggles_at_startup(); }
  };
  DiagTogglesStartupBanner g_diag_toggles_startup_banner;
}
