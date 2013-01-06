/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 * Based on ptp-noposix project (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_resp_calib_req(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	MsgSignaling wrsig_msg;

	if (ppi->is_new_state) {
		DSPOR(ppi)->wrPortState = WRS_RESP_CALIB_REQ;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
		if (DSPOR(ppi)->otherNodeCalSendPattern) {
			wr_calibration_pattern_enable(ppi, 0, 0, 0);
			pp_timer_start(
				DSPOR(ppi)->otherNodeCalPeriod / 1000,
				ppi->timers[PP_TIMER_WRS_RESP_CALIB_REQ]);
		}

	}

	if ((DSPOR(ppi)->otherNodeCalSendPattern) &&
	    (pp_timer_expired(ppi->timers[PP_TIMER_WRS_RESP_CALIB_REQ]))) {
		if (DSPOR(ppi)->wrMode == WR_MASTER)
			ppi->next_state = PPS_MASTER;
		else
			ppi->next_state = PPS_LISTENING;
		DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto state_updated;
	}

	if (plen == 0)
		goto state_updated;

	if (ppi->msg_tmp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(DSPOR(ppi)->msgTmpWrMessageID));

		if (DSPOR(ppi)->msgTmpWrMessageID == CALIBRATED) {
			if (DSPOR(ppi)->otherNodeCalSendPattern)
				wr_calibration_pattern_disable(ppi);
			if (DSPOR(ppi)->wrMode == WR_MASTER)
				ppi->next_state = WRS_WR_LINK_ON;
			else
				ppi->next_state = WRS_CALIBRATION;

		}
	}

state_updated:
	if (ppi->next_state != ppi->state)
		pp_timer_stop(ppi->timers[PP_TIMER_WRS_M_LOCK]);

	ppi->next_delay = DSPOR(ppi)->wrStateTimeout;

	return e;
}
