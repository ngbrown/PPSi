/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <pptp/pptp.h>
#include <pptp/diag.h>

/*
 * Fault troubleshooting. Now only prints an error messages and comes back to
 * PTP_INITIALIZING state
 */

int pp_faulty(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	pp_printf("event FAULT_CLEARED\n");
	ppi->next_state = PPS_INITIALIZING;
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;
}
