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
		DSPOR(ppi)->portState = PPS_UNCALIBRATED;
		DSPOR(ppi)->wrPortState = WRS_S_LOCK;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
		wr_locking_enable(ppi);
		pp_timer_start(WR_S_LOCK_TIMEOUT_MS,
			       ppi->timers[PP_TIMER_WRS_S_LOCK]);
	}

	if (pp_timer_expired(ppi->timers[PP_TIMER_WRS_S_LOCK])) {
		ppi->next_state = PPS_FAULTY;
		DSPOR(ppi)->wrPortState = WRS_IDLE;
		DSPOR(ppi)->wrMode = NON_WR;
		goto state_updated;
	}

	if (wr_locking_poll(ppi) == WR_SPLL_READY) {
		ppi->next_state = WRS_LOCKED;
		wr_locking_disable(ppi);
	}

state_updated:
	if (ppi->next_state != ppi->state)
		pp_timer_stop(ppi->timers[PP_TIMER_WRS_S_LOCK]);

	ppi->next_delay = DSPOR(ppi)->wrStateTimeout;

	return e;
}
