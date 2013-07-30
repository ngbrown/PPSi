/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_locked(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	MsgSignaling wrsig_msg;

	if (ppi->is_new_state) {
		WR_DSPOR(ppi)->wrPortState = WRS_LOCKED;
		pp_timeout_set(ppi, PP_TO_EXT_0,
			       WR_DSPOR(ppi)->wrStateTimeout);


		e = msg_issue_wrsig(ppi, LOCKED);
	}

	if (pp_timeout_z(ppi, PP_TO_EXT_0)) {
		ppi->next_state = PPS_LISTENING;
		WR_DSPOR(ppi)->wrMode = NON_WR;
		WR_DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto out;
	}

	if (plen == 0)
		goto out;

	if (ppi->received_ptp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(WR_DSPOR(ppi)->msgTmpWrMessageID));

		if (WR_DSPOR(ppi)->msgTmpWrMessageID == CALIBRATE)
			ppi->next_state = WRS_RESP_CALIB_REQ;
	}

out:
	if (e != 0)
		ppi->next_state = PPS_FAULTY;
	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;

	return e;
}
