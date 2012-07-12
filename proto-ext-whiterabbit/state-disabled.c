/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>

int pp_disabled(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	/* nothing to do */
	DSPOR(ppi)->portState = PPS_DISABLED;
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;
}
