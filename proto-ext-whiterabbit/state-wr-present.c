/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"
#include "../proto-standard/common-fun.h"

/*
 * WRS_PRESENT is the entry point for a WR slave
 *
 * Here we send SLAVE_PRESENT and wait for LOCK. If timeout,
 * resent SLAVE_PRESENT from WR_STATE_RETRY times
 */
int wr_present(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0, sendmsg = 0;
	struct wr_dsport *wrp = WR_DSPOR(ppi);

	MsgSignaling wrsig_msg;

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
		pp_timeout_set(ppi, PP_TO_EXT_0,
			       WR_WRS_PRESENT_TIMEOUT_MS);
		pp_timeout_restart_annrec(ppi);
		e = msg_issue_wrsig(ppi, SLAVE_PRESENT);
	}

	if (plen == 0)
		goto out;

	if (ppi->received_ptp_header.messageType == PPM_SIGNALING) {

		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(wrp->msgTmpWrMessageID));

		if (wrp->msgTmpWrMessageID == LOCK)
			ppi->next_state = WRS_S_LOCK;
	}

out:
	if (e == 0)
		st_com_execute_slave(ppi);
	else
		ppi->next_state = PPS_FAULTY;

	if (ppi->next_state != ppi->state)
		pp_timeout_clr(ppi, PP_TO_ANN_RECEIPT);

	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;

	return e;
}
