/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 * Based on ptp-noposix project (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_link_on(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;

	if (ppi->is_new_state) {
		WR_DSPOR(ppi)->wrModeOn = TRUE;
		wr_enable_ptracker(ppi);

		if (WR_DSPOR(ppi)->wrMode == WR_MASTER)
			e = msg_issue_wrsig(ppi, WR_MODE_ON);

		WR_DSPOR(ppi)->parentWrModeOn = TRUE;
		WR_DSPOR(ppi)->wrPortState = WRS_WR_LINK_ON;
	}

	if (e != 0)
		return -1;

	WR_DSPOR(ppi)->wrPortState = WRS_IDLE;

	if (WR_DSPOR(ppi)->wrMode == WR_SLAVE)
		ppi->next_state = PPS_SLAVE;
	else
		ppi->next_state = PPS_MASTER;

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;
}
