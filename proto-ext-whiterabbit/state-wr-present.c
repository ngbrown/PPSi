/*
 * Aurelio Colosimo for CERN, 2012 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_present(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;

	if (ppi->is_new_state) {
		DSPOR(ppi)->portState = PPS_UNCALIBRATED;
		DSPOR(ppi)->wrPortState = WRS_PRESENT;
		DSPOR(ppi)->wrMode = WR_SLAVE;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
		pp_timer_start(DSPOR(ppi)->wrStateTimeout / 1000,
			ppi->timers[PP_TIMER_WRS_PRESENT]);
		st_com_restart_annrec_timer(ppi);
		e = msg_issue_wrsig(ppi, SLAVE_PRESENT);
	}

	if (pp_timer_expired(ppi->timers[PP_TIMER_WRS_PRESENT])) {
		ppi->next_state = PPS_LISTENING;
		goto state_updated;
	}

	if (e == 0)
		st_com_execute_slave(ppi, 0);
	else
		ppi->next_state = PPS_FAULTY;

state_updated:
	if (ppi->next_state != ppi->state) {
		pp_timer_stop(ppi->timers[PP_TIMER_WRS_PRESENT]);
		pp_timer_stop(ppi->timers[PP_TIMER_ANN_RECEIPT]);
	}

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return e;
}
