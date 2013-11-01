/*
 * This work is part of the White Rabbit project
 *
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo <aurelio@aureliocolosimo.it>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */
#include <stdint.h>
#include <errno.h>
#include <ppsi/ppsi.h>
#include "wrpc.h"
#include "../proto-ext-whiterabbit/wr-api.h"
#include "../proto-ext-whiterabbit/wr-constants.h"

/* All of these live in wrpc-sw/include */
#include "minic.h"
#include "syscon.h"
#include "endpoint.h"
#include "softpll_ng.h"
#include "pps_gen.h"
#include "uart.h"
#include "rxts_calibrator.h"

extern int32_t cal_phase_transition;

int ptp_mode = WRC_MODE_UNKNOWN;
static int ptp_enabled = 0, ptp_forced_stop = 0;

static struct wr_operations wrpc_wr_operations = {
	.locking_enable = wrpc_spll_locking_enable,
	.locking_poll = wrpc_spll_locking_poll,
	.locking_disable = wrpc_spll_locking_disable,
	.enable_ptracker = wrpc_spll_enable_ptracker,

	.adjust_in_progress = wrpc_adjust_in_progress,
	.adjust_counters = wrpc_adjust_counters,
	.adjust_phase = wrpc_adjust_phase,

	.read_calib_data = wrpc_read_calibration_data,
	.calib_disable = wrpc_calibrating_disable,
	.calib_enable = wrpc_calibrating_enable,
	.calib_poll = wrpc_calibrating_poll,
	.calib_pattern_enable = wrpc_calibration_pattern_enable,
	.calib_pattern_disable = wrpc_calibration_pattern_disable,

	.enable_timing_output = wrpc_enable_timing_output,
};

/*ppi fields*/
static DSDefault  defaultDS;
static DSCurrent  currentDS;
static DSParent   parentDS;
static DSTimeProperties timePropertiesDS;
static struct pp_servo servo;

static struct wr_dsport wr_dsport = {
	.ops = &wrpc_wr_operations,
};
static DSPort     portDS = {
	.ext_dsport = &wr_dsport
};


static int delay_ms = PP_DEFAULT_NEXT_DELAY_MS;
static int start_tics = 0;
static int last_link_up = 0;

static struct pp_globals ppg_static; /* forward declaration */

/* despite the name, ppi_static is not static: tests/measure_t24p.c uses it */
struct pp_instance ppi_static = {
	.glbs			= &ppg_static,
	.portDS			= &portDS,
	.n_ops			= &wrpc_net_ops,
	.t_ops			= &wrpc_time_ops,
	.iface_name 	= "wr1",
};

/* We now have a structure with all globals, and multiple ppi inside */
static struct pp_globals ppg_static = {
	.pp_instances		= &ppi_static,
	.servo			= &servo,
	.defaultDS		= &defaultDS,
	.currentDS		= &currentDS,
	.parentDS		= &parentDS,
	.timePropertiesDS	= &timePropertiesDS,
};

int wrc_ptp_init()
{
	sdb_find_devices();
	uart_init_hw();
	uart_init_sw();

	pp_printf("PPSi for WRPC. Commit %s, built on " __DATE__ "\n",
		PPSI_VERSION);

	return 0;
}

#define LOCK_TIMEOUT_FM (4 * TICS_PER_SECOND)
#define LOCK_TIMEOUT_GM (60 * TICS_PER_SECOND)

int wrc_ptp_set_mode(int mode)
{
	uint32_t start_tics, lock_timeout = 0;
	struct pp_instance *ppi = &ppi_static;
	struct pp_globals *ppg = ppi->glbs;
	typeof(ppg->rt_opts->clock_quality.clockClass) *class_ptr;
	int error = 0;
	/*
	 * We need to change the class in the default options.
	 * Unfortunately, ppg->rt_opts may be yet unassigned when this runs
	 */
	class_ptr = &__pp_default_rt_opts.clock_quality.clockClass;

	ptp_mode = 0;

	wrc_ptp_stop();

	switch (mode) {
	case WRC_MODE_GM:
		WR_DSPOR(ppi)->wrConfig = WR_M_ONLY;
		ppi->master_only = TRUE;
		ppi->slave_only = FALSE;
		*class_ptr = PP_CLASS_WR_GM_LOCKED;
		spll_init(SPLL_MODE_GRAND_MASTER, 0, 1);
		lock_timeout = LOCK_TIMEOUT_GM;
		break;

	case WRC_MODE_MASTER:
		WR_DSPOR(ppi)->wrConfig = WR_M_ONLY;
		ppi->master_only = TRUE;
		ppi->slave_only = FALSE;
		*class_ptr = PP_CLASS_DEFAULT;
		spll_init(SPLL_MODE_FREE_RUNNING_MASTER, 0, 1);
		lock_timeout = LOCK_TIMEOUT_FM;
		break;

	case WRC_MODE_SLAVE:
		WR_DSPOR(ppi)->wrConfig = WR_S_ONLY;
		ppi->master_only = FALSE;
		ppi->slave_only = TRUE;
		*class_ptr = PP_CLASS_SLAVE_ONLY;
		spll_init(SPLL_MODE_SLAVE, 0, 1);
		break;
	}

	start_tics = timer_get_tics();

	pp_printf("Locking PLL");
	shw_pps_gen_enable_output(0);

	while (!spll_check_lock(0) && lock_timeout) {
		timer_delay(TICS_PER_SECOND);
		if (timer_get_tics() - start_tics > lock_timeout) {
			pp_printf("\nLock timeout.");
			error = -ETIMEDOUT;
			break;
		}
		pp_printf(".");
	}
	pp_printf("\n");

	/* If we can't lock to the atomic/gps, we say it in the class */
	if (error && mode == WRC_MODE_GM)
		*class_ptr = PP_CLASS_WR_GM_UNLOCKED;

	/* Success or failure, we enable pps */
	if (mode == WRC_MODE_MASTER || mode == WRC_MODE_GM)
		shw_pps_gen_enable_output(1);

	ptp_mode = mode;
	return error;
}

int wrc_ptp_get_mode()
{
	return ptp_mode;
}

int wrc_ptp_start()
{
	struct pp_instance *ppi = &ppi_static;

	pp_open_globals(&ppg_static);

	/* Call the state machine. Being it in "Initializing" state, make
	 * ppsi initialize what is necessary */
	delay_ms = pp_state_machine(ppi, NULL, 0);
	start_tics = timer_get_tics();

	WR_DSPOR(ppi)->linkUP = FALSE;
	wr_servo_reset();

	ptp_enabled = 1;
	return 0;
}

int wrc_ptp_stop()
{
	ptp_enabled = 0;
	wr_servo_reset();
	pp_close_globals(&ppg_static);
	return 0;
}

int wrc_ptp_update()
{
	int i;
	struct pp_instance *ppi = &ppi_static;

	int now_link_up;

	now_link_up = ep_link_up(NULL);

	if (last_link_up != now_link_up) {
		last_link_up = now_link_up;
		if (ptp_enabled && (!now_link_up)) {
			wrc_ptp_stop();
			ptp_forced_stop = 1;
			pp_printf("Link down: PTP stop\n");
		}
		else {
			if (ptp_forced_stop) {
				pp_printf("Link up: PTP start\n");
				ptp_forced_stop = 0;
				wrc_ptp_start();
			}
		}
	}

	if (!ptp_enabled)
		return 0;

	i = ppi->n_ops->recv(ppi, ppi->rx_frame, PP_MAX_FRAME_LENGTH - 4,
			     &ppi->last_rcv_time);

	if ((!i) && (timer_get_tics() - start_tics < delay_ms))
		return 0;

	if (!i) {
		/* Nothing received, but timeout elapsed */
		start_tics = timer_get_tics();
		delay_ms = pp_state_machine(ppi, NULL, 0);
		return 0;
	}
	delay_ms = pp_state_machine(ppi, ppi->rx_ptp, i);
	return 0;
}
