/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <pptp/pptp.h>
#include <pptp/diag.h>

/*
 * This has diagnostics. It calls pp_printf (which one, we don't know)
 */

void pp_diag_fsm(struct pp_instance *ppi, int sequence, int plen)
{
	if (!sequence) {
		/* enter with or without a packet len */
		pp_printf("fsm for %p: ENTER %3i (packet len %i)\n",
			  ppi, ppi->state, plen);
		return;
	}
	/* leave has one \n more, so different states are separate */
	pp_printf("fsm for %p: LEAVE %3i (next: %3i in %i ms)\n\n",
		  ppi, ppi->state, ppi->next_state, ppi->next_delay);
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
	char buf[128];

	va_start(args, fmt);
	pp_vsprintf(buf, fmt, args);
	va_end(args);
	pp_printf("MESSAGE for %p: %s", ppi, buf);
}
