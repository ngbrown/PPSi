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
 * We enter this state from  WRS_M_LOCK or WRS_RESP_CALIB_REQ.
 * We send CALIBRATE and do the hardware steps; finally we send CALIBRATED.
 */
int wr_calibration(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);
	int e = 0, sendmsg = 0;
	uint32_t delta;

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
			       wrp->calPeriod);
		e = msg_issue_wrsig(ppi, CALIBRATE);
		wrp->wrPortState = WR_PORT_CALIBRATION_0;
		if (wrp->calibrated)
			wrp->wrPortState = WR_PORT_CALIBRATION_2;
	}

	pp_diag(ppi, ext, 1, "%s: substate %i\n", __func__,
		wrp->wrPortState - WR_PORT_CALIBRATION_0);

	switch (wrp->wrPortState) {
	case WR_PORT_CALIBRATION_0:
		/* enable pattern sending */
		if (wrp->ops->calib_pattern_enable(ppi, 0, 0, 0) ==
			WR_HW_CALIB_OK)
			wrp->wrPortState = WR_PORT_CALIBRATION_1;
		else
			break;

	case WR_PORT_CALIBRATION_1:
		/* enable Tx calibration */
		if (wrp->ops->calib_enable(ppi, WR_HW_CALIB_TX)
				== WR_HW_CALIB_OK)
			wrp->wrPortState = WR_PORT_CALIBRATION_2;
		else
			break;

	case WR_PORT_CALIBRATION_2:
		/* wait until Tx calibration is finished */
		if (wrp->ops->calib_poll(ppi, WR_HW_CALIB_TX, &delta) ==
			WR_HW_CALIB_READY) {
			wrp->deltaTx.scaledPicoseconds.msb =
				0xFFFFFFFF & (((uint64_t)delta) >> 16);
			wrp->deltaTx.scaledPicoseconds.lsb =
				0xFFFFFFFF & (((uint64_t)delta) << 16);
			pp_diag(ppi, ext, 1, "Tx=>>scaledPicoseconds.msb = 0x%x\n",
				wrp->deltaTx.scaledPicoseconds.msb);
			pp_diag(ppi, ext, 1, "Tx=>>scaledPicoseconds.lsb = 0x%x\n",
				wrp->deltaTx.scaledPicoseconds.lsb);

			wrp->wrPortState = WR_PORT_CALIBRATION_3;
		} else {
			break; /* again */
		}

	case WR_PORT_CALIBRATION_3:
		/* disable Tx calibration */
		if (wrp->ops->calib_disable(ppi, WR_HW_CALIB_TX)
				== WR_HW_CALIB_OK)
			wrp->wrPortState = WR_PORT_CALIBRATION_4;
		else
			break;

	case WR_PORT_CALIBRATION_4:
		/* disable pattern sending */
		if (wrp->ops->calib_pattern_disable(ppi) == WR_HW_CALIB_OK)
			wrp->wrPortState = WR_PORT_CALIBRATION_5;
		else
			break;

	case WR_PORT_CALIBRATION_5:
		/* enable Rx calibration using the pattern sent by other port */
		if (wrp->ops->calib_enable(ppi, WR_HW_CALIB_RX) ==
				WR_HW_CALIB_OK)
			wrp->wrPortState = WR_PORT_CALIBRATION_6;
		else
			break;

	case WR_PORT_CALIBRATION_6:
		/* wait until Rx calibration is finished */
		if (wrp->ops->calib_poll(ppi, WR_HW_CALIB_RX, &delta) ==
			WR_HW_CALIB_READY) {
			pp_diag(ppi, ext, 1, "Rx fixed delay = %d\n", (int)delta);
			wrp->deltaRx.scaledPicoseconds.msb =
				0xFFFFFFFF & (delta >> 16);
			wrp->deltaRx.scaledPicoseconds.lsb =
				0xFFFFFFFF & (delta << 16);
			pp_diag(ppi, ext, 1, "Rx=>>scaledPicoseconds.msb = 0x%x\n",
				wrp->deltaRx.scaledPicoseconds.msb);
			pp_diag(ppi, ext, 1, "Rx=>>scaledPicoseconds.lsb = 0x%x\n",
				wrp->deltaRx.scaledPicoseconds.lsb);

			wrp->wrPortState = WR_PORT_CALIBRATION_7;
		} else {
			break; /* again */
		}

	case WR_PORT_CALIBRATION_7:
		/* disable Rx calibration */
		if (wrp->ops->calib_disable(ppi, WR_HW_CALIB_RX)
				== WR_HW_CALIB_OK)
			wrp->wrPortState = WR_PORT_CALIBRATION_8;
		else
			break;
	case WR_PORT_CALIBRATION_8:
		/* send deltas to the other port and go to the next state */
		e = msg_issue_wrsig(ppi, CALIBRATED);
		ppi->next_state = WRS_CALIBRATED;
		wrp->calibrated = TRUE;

	default:
		break;
	}

	ppi->next_delay = wrp->wrStateTimeout;

	return e;
}
