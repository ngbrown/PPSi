/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 * Based on ptp-noposix project (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_calibration(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	uint32_t delta;

	if (ppi->is_new_state) {
		WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION;

		e = msg_issue_wrsig(ppi, CALIBRATE);
		pp_timeout_set(ppi, PP_TO_EXT_0,
			       WR_DSPOR(ppi)->calPeriod);
		if (WR_DSPOR(ppi)->calibrated)
			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_2;
	}

	if (pp_timeout_z(ppi, PP_TO_EXT_0)) {
		if (WR_DSPOR(ppi)->wrMode == WR_MASTER)
			ppi->next_state = PPS_MASTER;
		else
			ppi->next_state = PPS_LISTENING;
		WR_DSPOR(ppi)->wrPortState = WRS_IDLE;
		goto out;
	}

	switch (WR_DSPOR(ppi)->wrPortState) {
	case WRS_CALIBRATION:
		/* enable pattern sending */
		if (wr_calibration_pattern_enable(ppi, 0, 0, 0) ==
			WR_HW_CALIB_OK)
			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_1;
		else
			break;

	case WRS_CALIBRATION_1:
		/* enable Tx calibration */
		if (wr_calibrating_enable(ppi, WR_HW_CALIB_TX)
				== WR_HW_CALIB_OK)
			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_2;
		else
			break;

	case WRS_CALIBRATION_2:
		/* wait until Tx calibration is finished */
		if (wr_calibrating_poll(ppi, WR_HW_CALIB_TX, &delta) ==
			WR_HW_CALIB_READY) {
			WR_DSPOR(ppi)->deltaTx.scaledPicoseconds.msb =
				0xFFFFFFFF & (((uint64_t)delta) >> 16);
			WR_DSPOR(ppi)->deltaTx.scaledPicoseconds.lsb =
				0xFFFFFFFF & (((uint64_t)delta) << 16);
			pp_diag(ppi, ext, 1, "Tx=>>scaledPicoseconds.msb = 0x%x\n",
				WR_DSPOR(ppi)->deltaTx.scaledPicoseconds.msb);
			pp_diag(ppi, ext, 1, "Tx=>>scaledPicoseconds.lsb = 0x%x\n",
				WR_DSPOR(ppi)->deltaTx.scaledPicoseconds.lsb);

			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_3;
		} else {
			break; /* again */
		}

	case WRS_CALIBRATION_3:
		/* disable Tx calibration */
		if (wr_calibrating_disable(ppi, WR_HW_CALIB_TX)
				== WR_HW_CALIB_OK)
			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_4;
		else
			break;

	case WRS_CALIBRATION_4:
		/* disable pattern sending */
		if (wr_calibration_pattern_disable(ppi) == WR_HW_CALIB_OK)
			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_5;
		else
			break;

	case WRS_CALIBRATION_5:
		/* enable Rx calibration using the pattern sent by other port */
		if (wr_calibrating_enable(ppi, WR_HW_CALIB_RX) ==
				WR_HW_CALIB_OK)
			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_6;
		else
			break;

	case WRS_CALIBRATION_6:
		/* wait until Rx calibration is finished */
		if (wr_calibrating_poll(ppi, WR_HW_CALIB_RX, &delta) ==
			WR_HW_CALIB_READY) {
			pp_diag(ppi, ext, 1, "Rx fixed delay = %d\n", (int)delta);
			WR_DSPOR(ppi)->deltaRx.scaledPicoseconds.msb =
				0xFFFFFFFF & (delta >> 16);
			WR_DSPOR(ppi)->deltaRx.scaledPicoseconds.lsb =
				0xFFFFFFFF & (delta << 16);
			pp_diag(ppi, ext, 1, "Rx=>>scaledPicoseconds.msb = 0x%x\n",
				WR_DSPOR(ppi)->deltaRx.scaledPicoseconds.msb);
			pp_diag(ppi, ext, 1, "Rx=>>scaledPicoseconds.lsb = 0x%x\n",
				WR_DSPOR(ppi)->deltaRx.scaledPicoseconds.lsb);

			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_7;
		} else {
			break; /* again */
		}

	case WRS_CALIBRATION_7:
		/* disable Rx calibration */
		if (wr_calibrating_disable(ppi, WR_HW_CALIB_RX)
				== WR_HW_CALIB_OK)
			WR_DSPOR(ppi)->wrPortState = WRS_CALIBRATION_8;
		else
			break;
	case WRS_CALIBRATION_8:
		/* send deltas to the other port and go to the next state */
		e = msg_issue_wrsig(ppi, CALIBRATED);
		ppi->next_state = WRS_CALIBRATED;
		WR_DSPOR(ppi)->calibrated = TRUE;

	default:
		break;
	}

out:
	ppi->next_delay = WR_DSPOR(ppi)->wrStateTimeout;

	return e;
}
