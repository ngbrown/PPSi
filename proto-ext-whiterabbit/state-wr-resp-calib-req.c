/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 * Based on ptp-noposix project (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_resp_calib_req(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	/* FIXME implementation */
	if (ppi->is_new_state) {
		DSPOR(ppi)->portState = PPS_SLAVE;
		DSPOR(ppi)->wrPortState = WRS_RESP_CALIB_REQ;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	}
	return 0;
}
