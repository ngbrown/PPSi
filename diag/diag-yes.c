/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

/*
 * This has diagnostics. It calls pp_printf (which one, we don't know)
 */

void pp_diag_error(struct pp_instance *ppi, int err)
{
	pp_printf("ERR for %p: %i\n", ppi, err);
}

void pp_diag_error_str2(struct pp_instance *ppi, char *s1, char *s2)
{
	pp_printf("ERR for %p: %s %s\n", ppi, s1, s2);
}

void pp_timed_printf(struct pp_instance *ppi, char *fmt, ...)
{
	va_list args;
	TimeInternal t;
	unsigned long oflags = pp_global_flags;

	/* temporarily set NOTIMELOG, as we'll print the time ourselves */
	pp_global_flags |= PP_FLAG_NOTIMELOG;
	ppi->t_ops->get(&t);
	pp_global_flags = oflags;

	pp_printf("%09d.%03d ", (int)t.seconds,
		  (int)t.nanoseconds / 1000000);
	va_start(args, fmt);
	pp_vprintf(fmt, args);
	va_end(args);
}
