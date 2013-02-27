/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 */

#include <stdint.h>
#include <ppsi/ppsi.h>
#include <pps_gen.h>
#include <softpll_ng.h>
#include "../proto-ext-whiterabbit/wr-constants.h"

int wrpc_spll_locking_enable(struct pp_instance *ppi)
{
	spll_init(SPLL_MODE_SLAVE, 0, 1);
	spll_enable_ptracker(0, 1);
	return WR_SPLL_OK;
}

int wrpc_spll_locking_poll(struct pp_instance *ppi)
{
	return spll_check_lock(0) ? WR_SPLL_READY : WR_SPLL_ERROR;
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
	shw_pps_gen_enable_output(enable);
	return WR_SPLL_OK;
}

int wrpc_adjust_in_progress()
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

int wr_locking_enable(struct pp_instance *ppi)
	__attribute__((alias("wrpc_spll_locking_enable")));

int wr_locking_poll(struct pp_instance *ppi)
	__attribute__((alias("wrpc_spll_locking_poll")));

int wr_locking_disable(struct pp_instance *ppi)
	__attribute__((alias("wrpc_spll_locking_disable")));

int wr_enable_ptracker(struct pp_instance *ppi)
	__attribute__((alias("wrpc_spll_enable_ptracker")));

int wr_enable_timing_output(struct pp_instance *ppi, int enable)
	__attribute__((alias("wrpc_enable_timing_output")));

int wr_adjust_in_progress()
	__attribute__((alias("wrpc_adjust_in_progress")));

int wr_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec)
	__attribute__((alias("wrpc_adjust_counters")));

int wr_adjust_phase(int32_t phase_ps)
	__attribute__((alias("wrpc_adjust_phase")));
