/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released to the public domain
 */

#include <stdint.h>
#include <ppsi/ppsi.h>
#include <pps_gen.h>
#include <softpll_ng.h>
#include "../proto-ext-whiterabbit/wr-constants.h"
#include <rxts_calibrator.h>
#include "wrpc.h"

extern uint32_t cal_phase_transition;

int wrpc_spll_locking_enable(struct pp_instance *ppi)
{
	spll_init(SPLL_MODE_SLAVE, 0, 1);
	spll_enable_ptracker(0, 1);
	return WR_SPLL_OK;
}

int wrpc_spll_locking_poll(struct pp_instance *ppi, int grandmaster)
{
	int locked;
	static int t24p_calibrated = 0;

	locked = spll_check_lock(0); /* both slave and gm mode */

	if (grandmaster)
		return locked ? WR_SPLL_READY : WR_SPLL_ERROR;

	/* Else, slave: ensure calibration is done */
	if(!locked) {
		t24p_calibrated = 0;
	}
	else if(locked && !t24p_calibrated) {
		/*run t24p calibration if needed*/
		calib_t24p(WRC_MODE_SLAVE, &cal_phase_transition);
		t24p_calibrated = 1;
	}

	return locked ? WR_SPLL_READY : WR_SPLL_ERROR;
}

int wrpc_spll_locking_disable(struct pp_instance *ppi)
{
	/* softpll_disable(); */
	return WR_SPLL_OK;
}

int wrpc_spll_enable_ptracker(struct pp_instance *ppi)
{
	spll_enable_ptracker(0, 1);
	return WR_SPLL_OK;
}

int wrpc_enable_timing_output(struct pp_instance *ppi, int enable)
{
	if (enable == WR_DSPOR(ppi)->ppsOutputOn)
		return WR_SPLL_OK;
	WR_DSPOR(ppi)->ppsOutputOn = enable;

	shw_pps_gen_enable_output(enable);
	return WR_SPLL_OK;
}

int wrpc_adjust_in_progress(void)
{
	return shw_pps_gen_busy() || spll_shifter_busy(0);
}

int wrpc_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec)
{
	if (adjust_sec)
		shw_pps_gen_adjust(PPSG_ADJUST_SEC, adjust_sec);
	if (adjust_nsec)
		shw_pps_gen_adjust(PPSG_ADJUST_NSEC, adjust_nsec);
	return 0;
}

int wrpc_adjust_phase(int32_t phase_ps)
{
	spll_set_phase_shift(SPLL_ALL_CHANNELS, phase_ps);
	return WR_SPLL_OK;
}

