/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_calibrated(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	MsgSignaling wrsig_msg;

	if (ppi->is_new_state) {
		WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATED;
		pp_timeout_set(ppi, PP_TO_EXT_0,
			       WR_DSPOR(ppi)->wrStateTimeout);
	}

	if (pp_timeout_z(ppi, PP_TO_EXT_0)) {
		if (WR_DSPOR(ppi)->wrMode == WR_MASTER)
			ppi->next_state = PPS_MASTER;
		else
			ppi->next_state = PPS_LISTENING;
		WR_DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto out;
	}

	if (plen == 0)
		goto out;

	if (ppi->received_ptp_header.messageType == PPM_SIGNALING) {
		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(WR_DSPOR(ppi)->msgTmpWrMessageID));

		if ((WR_DSPOR(ppi)->msgTmpWrMessageID == CALIBRATE) &&
			(WR_DSPOR(ppi)->wrMode == WR_MASTER))
			ppi->next_state = WRS_RESP_CALIB_REQ;
		else if ((WR_DSPOR(ppi)->msgTmpWrMessageID == WR_MODE_ON) &&
			(WR_DSPOR(ppi)->wrMode == WR_SLAVE))
			ppi->next_state = WRS_WR_LINK_ON;
	}

out:
	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;
	return 0;
}
