/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 * Based on ptp-noposix project (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_locked(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	MsgSignaling wrsig_msg;

	if (ppi->is_new_state) {
		DSPOR(ppi)->portState = PPS_UNCALIBRATED;
		DSPOR(ppi)->wrPortState = WRS_LOCKED;
		pp_timer_start(DSPOR(ppi)->wrStateTimeout / 1000,
			ppi->timers[PP_TIMER_WRS_LOCKED]);

		e = msg_issue_wrsig(ppi, LOCKED);
	}

	if (pp_timer_expired(ppi->timers[PP_TIMER_WRS_LOCKED])) {
		ppi->next_state = PPS_LISTENING;
		DSPOR(ppi)->wrMode = NON_WR;
		DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto state_updated;
	}

	if (plen == 0)
		goto no_incoming_msg;

	if (ppi->msg_tmp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(DSPOR(ppi)->msgTmpWrMessageID));

		if (DSPOR(ppi)->msgTmpWrMessageID == CALIBRATE)
			ppi->next_state = WRS_RESP_CALIB_REQ;
	}

no_incoming_msg:
	if (e != 0)
		ppi->next_state = PPS_FAULTY;

state_updated:
	if (ppi->next_state != ppi->state)
		pp_timer_stop(ppi->timers[PP_TIMER_WRS_LOCKED]);

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return e;
}
