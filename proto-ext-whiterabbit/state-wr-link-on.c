/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 * Based on ptp-noposix project (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_link_on(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	/* FIXME implementation */
	if (ppi->is_new_state) {
		/* DSPOR(ppi)->portState depends on previous states, should not
		 * change when entering in link on, so no need to update it here
		 */
		DSPOR(ppi)->wrPortState = WRS_WR_LINK_ON;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	}
	return 0;
}
