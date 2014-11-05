/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

/*
 * This the entry point for a WR master: send "LOCK" and wait
 * for "LOCKED". On timeout retry sending, for WR_STATE_RETRY times.
 */
int wr_m_lock(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0, sendmsg = 0;
	MsgSignaling wrsig_msg;
	struct wr_dsport *wrp = WR_DSPOR(ppi);

	if (ppi->is_new_state) {
		wrp->wrStateRetry = WR_STATE_RETRY;
		sendmsg = 1;
	} else if (pp_timeout_z(ppi, PP_TO_EXT_0)) {
		if (wr_handshake_retry(ppi))
			sendmsg = 1;
		else
			return 0; /* non-wr already */
	}

	if (sendmsg) {
		e = msg_issue_wrsig(ppi, LOCK);
		pp_timeout_set(ppi, PP_TO_EXT_0, WR_M_LOCK_TIMEOUT_MS);
	}

	if (plen == 0)
		goto out;

	if (ppi->received_ptp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(wrp->msgTmpWrMessageID));

		if (wrp->msgTmpWrMessageID == LOCKED)
			ppi->next_state = WRS_CALIBRATION;
	}

out:
	if (e != 0)
		ppi->next_state = PPS_FAULTY;
	ppi->next_delay = wrp->wrStateTimeout;

	return e;
}
