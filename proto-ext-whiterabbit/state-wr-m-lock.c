/*
 * Aurelio Colosimo for CERN, 2012 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_m_lock(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	MsgSignaling wrsig_msg;

	if (ppi->is_new_state) {
		DSPOR(ppi)->portState = PPS_MASTER;
		DSPOR(ppi)->wrPortState = WRS_M_LOCK;
		DSPOR(ppi)->wrMode = WR_MASTER;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
		e = msg_issue_wrsig(ppi, LOCK);
		pp_timer_start(WR_M_LOCK_TIMEOUT_MS / 1000,
			ppi->timers[PP_TIMER_WRS_M_LOCK]);
	}

	if (pp_timer_expired(ppi->timers[PP_TIMER_WRS_M_LOCK])) {
		ppi->next_state = PPS_MASTER;
		DSPOR(ppi)->wrPortState = WRS_IDLE;
		DSPOR(ppi)->wrMode = NON_WR;
		goto state_updated;
	}

	if (plen == 0)
		goto no_incoming_msg;

	if (ppi->msg_tmp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(DSPOR(ppi)->msgTmpWrMessageID));

		if (DSPOR(ppi)->msgTmpWrMessageID == LOCKED)
			ppi->next_state = WRS_CALIBRATION;
	}

no_incoming_msg:
	if (e != 0)
		ppi->next_state = PPS_FAULTY;

state_updated:
	if (ppi->next_state != ppi->state)
		pp_timer_stop(ppi->timers[PP_TIMER_WRS_M_LOCK]);

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return e;
}
