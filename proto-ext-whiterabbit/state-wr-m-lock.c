/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_m_lock(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	MsgSignaling wrsig_msg;

	if (ppi->is_new_state) {
		WR_DSPOR(ppi)->wrPortState = WRS_M_LOCK;
		WR_DSPOR(ppi)->wrMode = WR_MASTER;
		e = msg_issue_wrsig(ppi, LOCK);
		pp_timeout_set(ppi, PP_TO_EXT_0, WR_M_LOCK_TIMEOUT_MS);
	}

	if (pp_timeout_z(ppi, PP_TO_EXT_0)) {
		ppi->next_state = PPS_MASTER;
		WR_DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto out;
	}

	if (plen == 0)
		goto out;

	if (ppi->received_ptp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(WR_DSPOR(ppi)->msgTmpWrMessageID));

		if (WR_DSPOR(ppi)->msgTmpWrMessageID == LOCKED)
			ppi->next_state = WRS_CALIBRATION;
	}

out:
	if (e != 0)
		ppi->next_state = PPS_FAULTY;
	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;

	return e;
}
