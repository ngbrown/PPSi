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
 * We enter here from WRS_CALIBRATION.  If master we wait for
 * a CALIBRATE message, if slave we wait for LINK_ON.
 */
int wr_calibrated(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);
	MsgSignaling wrsig_msg;

	if (ppi->is_new_state)
		pp_timeout_set(ppi, PP_TO_EXT_0, wrp->wrStateTimeout);

	if (pp_timeout_z(ppi, PP_TO_EXT_0)) {
		/*
		 * FIXME: We should implement a retry by re-sending
		 * the "calibrated" message, moving it here from the
		 * previous state (sub-state 8 of "state-wr-calibration"
		 */
		wr_handshake_fail(ppi);
		return 0; /* non-wr */
	}

	if (plen == 0)
		goto out;

	if (ppi->received_ptp_header.messageType == PPM_SIGNALING) {
		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(wrp->msgTmpWrMessageID));

		if ((wrp->msgTmpWrMessageID == CALIBRATE) &&
			(wrp->wrMode == WR_MASTER))
			ppi->next_state = WRS_RESP_CALIB_REQ;
		else if ((wrp->msgTmpWrMessageID == WR_MODE_ON) &&
			(wrp->wrMode == WR_SLAVE))
			ppi->next_state = WRS_WR_LINK_ON;
	}

out:
	ppi->next_delay = wrp->wrStateTimeout;
	return 0;
}
