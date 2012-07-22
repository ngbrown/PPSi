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
		DSPOR(ppi)->wrPortState = WRS_CALIBRATED;
		ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	}

	if (plen == 0)
		goto ret;

	if (ppi->msg_tmp_header.messageType == PPM_SIGNALING) {
		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
			 &(DSPOR(ppi)->msgTmpWrMessageID));

		if ((DSPOR(ppi)->msgTmpWrMessageID == CALIBRATE) &&
			(DSPOR(ppi)->wrMode == WR_MASTER))
			ppi->next_state = WRS_RESP_CALIB_REQ;
		else if ((DSPOR(ppi)->msgTmpWrMessageID == WR_MODE_ON) &&
			(DSPOR(ppi)->wrMode == WR_SLAVE))
			ppi->next_state = WRS_WR_LINK_ON;
	}

ret:
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;
}
