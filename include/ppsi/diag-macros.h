/*
 * Macros for diagnostic prints.
 */


#define pp_error(...) pp_printf("ERROR: " __VA_ARGS__)

/*
 * The "new" diagnostics is based on flags: there are per-instance d_flags
 * and global d_flags.
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
	pp_dt_config	= 1,
};
/*
 * Note: we may use less bits and have more things, without changing
 * anything. Set's say levels are ony 0..3 -- still,I prefer to be
 * able to print the active d_flags as %x while debugging this very
 * mechanism).
 */

extern unsigned long pp_global_d_flags; /* Supplement ppi-specific ones */

#define PP_FLAG_NOTIMELOG      1 /* This is for a special case, I'm sorry */


/* So, extract the level */
#define __PP_FLAGS(ppi) ((ppi ? ppi->d_flags : 0) | pp_global_d_flags)

#define __PP_DIAG_ALLOW(ppi, th, level) \
		((__PP_FLAGS(ppi) >> (4 * (th)) & 0xf) >= level)

#define __PP_DIAG_ALLOW_FLAGS(f, th, level) \
		((f >> (4 * (th)) & 0xf) >= level)



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

#define pp_diag_allow(ppi_, th_, level_) \
		(PP_HAS_DIAG && __PP_DIAG_ALLOW(ppi_,  pp_dt_ ## th_, level_))

/*
 * And this is the parser of the string. Internally it obeys VERB_LOG_MESGS
 * to set the value to 0xfffffff0 to be compatible with previous usage.
 */
extern unsigned long pp_diag_parse(char *diaglevel);
