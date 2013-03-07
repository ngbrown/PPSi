/*
 * Macros for diagnostic prints.
 */

/*
 * We still have support for compile-time settings, in order to remove
 * quite a few kilobytes of stuff from the compiled binary. This first
 * part of the file is the previous way, which is still used in
 * several files. I plan to phase it out slowly, but let's avoid
 * massive changes at this point.
 *
 * This verbosity argument, that can be changed at run time
 * or not. If it can be changed, stuff works like this, depending on
 * an integer value somewhere (it is incremented by each -V on a host run).
 *
 * BTW, Host build always have CONFIG_PPSI_RUNTIME_VERBOSITY set, because we
 * have no size problems there (bare build don't have it, in order to
 * easily test how stuff work on real freestanding environments.
 */

#ifdef CONFIG_PPSI_RUNTIME_VERBOSITY
  #define CONST_VERBOSITY /* nothing: use "int pp_diag_verbosity" */

  #define PP_PRINTF(...) if (pp_diag_verbosity) pp_printf(__VA_ARGS__)
  #define PP_VPRINTF(...) if (pp_diag_verbosity > 1) pp_printf(__VA_ARGS__)

#else /* no runtime verbosity */

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
 * The following ones are expected to overcome the above uppercase macros.
 * We really need to remove code from the object files, but the finer-grained
 * selection (pp_verbose_dump etc) is a better long-term solution than
 * CONFIG_PPSI_RUNTIME_VERBOSITY
 */
#define pp_error(...) \
	if (pp_verbose_error && pp_diag_verbosity) pp_printf(__VA_ARGS__)

#define pp_Vprintf(...) \
	if (pp_diag_verbosity > 1) pp_printf(__VA_ARGS__)

/*
 * The "new" diagnostics is based on flags: there are per-instance flags
 * and global flags.
 *
 * Basically, we may have just bits about what to print, but we might
 * want to add extra-verbose stuff for specific cases, like looking at
 * how WR synchronizes, or all details of delay calculations.  So
 * let's use 4 bits for each "kind" of thing.
 *
 * By using 4 bits, we can make a simple parser (that can be used in
 * freestanding environments, without strtok or such) and have a
 * format like "00105", which is not really hex, because position 0 is
 * position 0, and you can omit trailing zeroes, not leading ones. The
 * parser is exported, so every arch uses the same convention.  By
 * doing like this we can have up to 7 "things", separate for each pp
 * instance (the low 4 bits are general diagnostic flags).
 *
 * So, let's name the things, numberinf from 7 downwards because we
 * think of the final hex number:
 */
enum pp_diag_things {
	pp_dt_fsm	= 7,
	pp_dt_time	= 6, /* level 1: set and timeouts, 2: get_time too */
	pp_dt_frames	= 5, /* level 1: send/recv, 2: dump contents */
	pp_dt_servo	= 4,
	pp_dt_bmc	= 3,
	pp_dt_ext	= 2,
};
/*
 * Note: we may use less bits and have more things, without changing
 * anything. Set's say levels are ony 0..3 -- still,I prefer to be
 * able to print the active flags as %x while debugging this very
 * mechanism).
 */

extern unsigned long pp_global_flags; /* Supplement ppi-specific ones */

#define PP_FLAG_NOTIMELOG      1 /* This is for a special case, I'm sorry */


/* So, extract the level */
#define __PP_FLAGS(ppi) (ppi->flags | pp_global_flags)

#define __PP_DIAG_ALLOW(ppi, th, level) \
		((__PP_FLAGS(ppi) >> (4 * (th)) & 0xf) >= level)

/* The function it is better external, since it is variadic */
extern void __pp_diag(struct pp_instance *ppi, enum pp_diag_things th,
		      int level, char *fmt, ...)
	__attribute__((format(printf, 4, 5)));

/* Now, *still* use PPSI_NO_DIAG as an escape route to kill all diag code */
#ifdef PPSI_NO_DIAG
#  define PP_HAS_DIAG 0
#else
#  define PP_HAS_DIAG 1
#endif

/* So, this is the function that is to be used by real ppsi code */
#define pp_diag(ppi_, th_, level_, ...)					\
	({								\
	if (PP_HAS_DIAG)						\
		__pp_diag(ppi_, pp_dt_ ## th_, level_, __VA_ARGS__);	\
	PP_HAS_DIAG; /* return 1 if done, 0 if not done */		\
	})

/*
 * And this is the parser of the string. Internally it obeys VERB_LOG_MESGS
 * to set the value to 0xfffffff0 to be compatible with previous usage.
 */
extern unsigned long pp_diag_parse(char *diaglevel);


/*
 * The following stuff is being removed in a few commits. It's stuff that
 * I did but don't like any more (because we really need per-instance bits
 */


/*
 * Then, we have this VERB_LOG_MSGS that used to be an ifdef. Provide
 * constants instead, to avoid the hairyness of ifdef.
 */
#ifdef VERB_LOG_MSGS
#define pp_verbose_error 1
#define pp_verbose_dump 1
#define pp_verbose_servo 1
#define pp_verbose_time 1
#endif

/* Accept 5 individual flags to turn on each of them */
#ifdef VERB_ERR
#define pp_verbose_error 1
#endif

#ifdef VERB_DUMP
#define pp_verbose_dump 1
#endif

#ifdef VERB_SERVO
#define pp_verbose_servo 1
#endif

#ifdef VERB_TIME
#define pp_verbose_time 1
#endif

/* Provide 0 as default for all such values */
#ifndef pp_verbose_error
#define pp_verbose_error 0
#endif

#ifndef pp_verbose_dump
#define pp_verbose_dump 0
#endif

#ifndef pp_verbose_servo
#define pp_verbose_servo 0
#endif

#ifndef pp_verbose_time
#define pp_verbose_time 0
#endif
