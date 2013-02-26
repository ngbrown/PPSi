/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 * Based on ptp-noposix project (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_calibrated(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	MsgSignaling wrsig_msg;

	if (ppi->is_new_state) {
		WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATED;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
		pp_timer_start(WR_DSPOR(ppi)->wrStateTimeout,
			ppi->timers[PP_TIMER_WRS_CALIBRATED]);
	}

	if (pp_timer_expired(ppi->timers[PP_TIMER_WRS_CALIBRATED])) {
		if (WR_DSPOR(ppi)->wrMode == WR_MASTER)
			ppi->next_state = PPS_MASTER;
		else
			ppi->next_state = PPS_LISTENING;
		WR_DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto state_updated;
	}

	if (plen == 0)
		goto ret;

	if (ppi->msg_tmp_header.messageType == PPM_SIGNALING) {
		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(WR_DSPOR(ppi)->msgTmpWrMessageID));

		if ((WR_DSPOR(ppi)->msgTmpWrMessageID == CALIBRATE) &&
			(WR_DSPOR(ppi)->wrMode == WR_MASTER))
			ppi->next_state = WRS_RESP_CALIB_REQ;
		else if ((WR_DSPOR(ppi)->msgTmpWrMessageID == WR_MODE_ON) &&
			(WR_DSPOR(ppi)->wrMode == WR_SLAVE))
			ppi->next_state = WRS_WR_LINK_ON;
	}

state_updated:
	if (ppi->next_state != ppi->state)
		pp_timer_stop(ppi->timers[PP_TIMER_WRS_CALIBRATED]);

ret:
	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;
	return 0;
}
