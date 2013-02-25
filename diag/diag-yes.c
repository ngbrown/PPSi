/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

/*
 * This has diagnostics. It calls pp_printf (which one, we don't know)
 */

void pp_diag_fsm(struct pp_instance *ppi, char *name, int sequence, int plen)
{
	if (sequence == STATE_ENTER) {
		/* enter with or without a packet len */
		pp_timed_printf("fsm: ENTER %s, packet len %i\n",
			  name, plen);
		return;
	}
	/* leave has one \n more, so different states are separate */
	pp_timed_printf("fsm: LEAVE %s (next: %3i in %i ms)\n\n",
		name, ppi->next_state, ppi->next_delay);
}

void pp_diag_trace(struct pp_instance *ppi, const char *f, int line)
{
	pp_printf("TRACE for %p: %s:%i\n", ppi, f, line);
}

void pp_diag_error(struct pp_instance *ppi, int err)
{
	pp_printf("ERR for %p: %i\n", ppi, err);
}

void pp_diag_error_str2(struct pp_instance *ppi, char *s1, char *s2)
{
	pp_printf("ERR for %p: %s %s\n", ppi, s1, s2);
}

void pp_diag_fatal(struct pp_instance *ppi, char *s1, char *s2)
{
	pp_printf("FATAL for %p: %s %s\n", ppi, s1, s2);
	while (1)
		/* nothing more */;
}

void pp_diag_printf(struct pp_instance *ppi, char *fmt, ...)
{
	va_list args;

	pp_printf("MESSAGE for %p: ", ppi);
	va_start(args, fmt);
	pp_vprintf(fmt, args);
	va_end(args);
}

void pp_timed_printf(char *fmt, ...)
{
	va_list args;
	TimeInternal t;

	pp_get_tstamp(&t);
	pp_printf("%09d.%03d ", (int)t.seconds,
		  (int)t.nanoseconds / 1000000);
	va_start(args, fmt);
	pp_vprintf(fmt, args);
	va_end(args);
}
