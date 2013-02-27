/*
 * Macros for diagnostic prints. The thing is not trivial because
 * we need to remove all strings and parameter passing when running
 * wrpc-sw with all its options enabled, for space reasons.
 *
 * Note that by building with PPSI_NO_DIAG (see diag/Makefile) no message
 * at all is there (and it can save quite some binary space).
 */

/*
 * Then we have a verbosity argument, that can be changed at run time
 * or not. If it can be changed, stuff works like this, depending on
 * an integer value somewhere (it is incremented by each -V on a host run)
 */

#ifdef CONFIG_PPSI_RUNTIME_VERBOSITY
#define CONST_VERBOSITY /* nothing: use "int pp_diag_verbosity" */
extern int pp_diag_verbosity;

#define PP_PRINTF(...) if (pp_diag_verbosity) pp_printf(__VA_ARGS__)
#define PP_VPRINTF(...) if (pp_diag_verbosity > 1) pp_printf(__VA_ARGS__)

#else

/*
 * If verbosity cannot be changed at run time, we have CONFIG_PPSI_VERBOSITY
 * set at build time (or at Kconfig time when we add it)
 */

#define CONST_VERBOSITY const /* use "const int pp_diag_verbosity */

  #if CONFIG_PPSI_VERBOSITY > 0
  #define PP_PRINTF(...) if (pp_diag_verbosity) pp_printf(__VA_ARGS__)
  #define PP_VPRINTF(...) if (pp_diag_verbosity > 1) pp_printf(__VA_ARGS__)
  #else
  #define PP_PRINTF(...)
  #define PP_VPRINTF(...)
  #endif /* CONFIG_PPSI_VERBOSITY > 0 */

#endif /* CONFIG_PPSI_RUNTIME_VERBOSITY */

extern CONST_VERBOSITY int pp_diag_verbosity;

/*
 * Then, we have this VERB_LOG_MSGS that used to be an ifdef. Provide
 * constants instead, to avoid the hairyness of ifdef.
 */
#ifdef VERB_LOG_MSGS
#define pp_verbose_dump 1
#define pp_verbose_servo 1
#define pp_verbose_frames 1
#define pp_verbose_time 1
#endif

/* Accept 4 individual flages to turn on each of them */
#ifdef VERB_DUMP
#define pp_verbose_dump 1
#endif

#ifdef VERB_SERVO
#define pp_verbose_servo 1
#endif

#ifdef VERB_FRAMES
#define pp_verbose_frames 1
#endif

#ifdef VERB_TIME
#define pp_verbose_time 1
#endif

/* Provide 0 as default for all such values */
#ifndef pp_verbose_dump
#define pp_verbose_dump 0
#endif

#ifndef pp_verbose_servo
#define pp_verbose_servo 0
#endif

#ifndef pp_verbose_frames
#define pp_verbose_frames 0
#endif

#ifndef pp_verbose_time
#define pp_verbose_time 0
#endif
