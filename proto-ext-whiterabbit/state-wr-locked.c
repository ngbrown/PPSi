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
		WR_DSPOR(ppi)->wrPortState = WRS_LOCKED;
		pp_timeout_set(ppi, PP_TO_EXT_0,
			       WR_DSPOR(ppi)->wrStateTimeout);


		e = msg_issue_wrsig(ppi, LOCKED);
	}

	if (pp_timeout(ppi, PP_TO_EXT_0)) {
		ppi->next_state = PPS_LISTENING;
		WR_DSPOR(ppi)->wrMode = NON_WR;
		WR_DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto state_updated;
	}

	if (plen == 0)
		goto no_incoming_msg;

	if (ppi->msg_tmp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(WR_DSPOR(ppi)->msgTmpWrMessageID));

		if (WR_DSPOR(ppi)->msgTmpWrMessageID == CALIBRATE)
			ppi->next_state = WRS_RESP_CALIB_REQ;
	}

no_incoming_msg:
	if (e != 0)
		ppi->next_state = PPS_FAULTY;

state_updated:
	if (ppi->next_state != ppi->state)
		pp_timeout_clr(ppi, PP_TO_EXT_0);

	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;

	return e;
}
