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
		WR_DSPOR(ppi)->wrPortState = WRS_RESP_CALIB_REQ;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
		if (WR_DSPOR(ppi)->otherNodeCalSendPattern) {
			wr_calibration_pattern_enable(ppi, 0, 0, 0);
			pp_timeout_set(ppi, PP_TO_WRS_RESP_CALIB_REQ,
			       WR_DSPOR(ppi)->otherNodeCalPeriod / 1000);
		}

	}

	if ((WR_DSPOR(ppi)->otherNodeCalSendPattern) &&
	    (pp_timeout(ppi, PP_TO_WRS_RESP_CALIB_REQ))) {
		if (WR_DSPOR(ppi)->wrMode == WR_MASTER)
			ppi->next_state = PPS_MASTER;
		else
			ppi->next_state = PPS_LISTENING;
		WR_DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto state_updated;
	}

	if (plen == 0)
		goto state_updated;

	if (ppi->msg_tmp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(WR_DSPOR(ppi)->msgTmpWrMessageID));

		if (WR_DSPOR(ppi)->msgTmpWrMessageID == CALIBRATED) {
			if (WR_DSPOR(ppi)->otherNodeCalSendPattern)
				wr_calibration_pattern_disable(ppi);
			if (WR_DSPOR(ppi)->wrMode == WR_MASTER)
				ppi->next_state = WRS_WR_LINK_ON;
			else
				ppi->next_state = WRS_CALIBRATION;

		}
	}

state_updated:
	if (ppi->next_state != ppi->state)
		pp_timeout_clr(ppi, PP_TO_WRS_M_LOCK);

	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;

	return e;
}
