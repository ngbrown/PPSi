/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>

/*
 * Fault troubleshooting. Now only prints an error messages and comes back to
 * PTP_INITIALIZING state
 */

int pp_faulty(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	DSPOR(ppi)->portState = PPS_FAULTY;
	PP_PRINTF("Faulty state detected\n");
	ppi->next_state = PPS_INITIALIZING;
	return 0;
}
