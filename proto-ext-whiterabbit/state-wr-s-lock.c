/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 * Based on ptp-noposix project (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_s_lock(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	if (ppi->is_new_state) {
		WR_DSPOR(ppi)->wrPortState = WRS_S_LOCK;
		wr_locking_enable(ppi);
		pp_timeout_set(ppi, PP_TO_EXT_0, WR_S_LOCK_TIMEOUT_MS);
	}

	if (pp_timeout_z(ppi, PP_TO_EXT_0)) {
		ppi->next_state = PPS_FAULTY;
		WR_DSPOR(ppi)->wrPortState = WRS_IDLE;
		WR_DSPOR(ppi)->wrMode = NON_WR;
		goto out;
	}

	if (wr_locking_poll(ppi) == WR_SPLL_READY) {
		ppi->next_state = WRS_LOCKED;
		wr_locking_disable(ppi);
	}

out:
	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;
	return e;
}
