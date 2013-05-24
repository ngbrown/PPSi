/*
 * Aurelio Colosimo for CERN, 2013 -- public domain
 */

#include <ppsi/ppsi.h>
#include <unix-time.h>
#include <minipc.h>

#define HAL_EXPORT_STRUCTURES
#include <hal_exports.h>

#define DEFAULT_TO 200000 /* ms */

extern struct minipc_ch *hal_ch;

int wrs_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec)
{
	hexp_pps_params_t p;
	int cmd;
	int ret, rval;

	if (!adjust_nsec && !adjust_sec)
		return 0;

	if (adjust_sec && adjust_nsec) {
		pp_printf("FATAL: trying to adjust both the SEC and the NS"
			" counters simultaneously. \n");
		exit(-1);
	}

	p.adjust_sec = adjust_sec;
	p.adjust_nsec = adjust_nsec;
	cmd = (adjust_sec ? HEXP_PPSG_CMD_ADJUST_SEC : HEXP_PPSG_CMD_ADJUST_NSEC);

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd, &rval, cmd, &p);

	if (ret < 0)
		return ret;

	return rval;
}

int wrs_adjust_phase(int32_t phase_ps)
{
	hexp_pps_params_t p;
	int ret, rval;
	p.adjust_phase_shift = phase_ps;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd,
		&rval, HEXP_PPSG_CMD_ADJUST_PHASE, &p);

	if (ret < 0)
		return ret;

	return rval;
}

int wrs_adjust_in_progress(void)
{
	hexp_pps_params_t p;
	int ret, rval;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd,
		&rval, HEXP_PPSG_CMD_ADJUST_PHASE, &p);

	return (ret == 0) && rval;
}

int wrs_enable_ptracker(struct pp_instance *ppi)
{
	/* FIXME wrs_enable_ptracker */
	return 0;
}

int wrs_enable_timing_output(struct pp_instance *ppi, int enable)
{
	/* FIXME wrs_enable_timing_output */
	return 0;
}

int wrs_locking_disable(struct pp_instance *ppi)
{
	/* FIXME wrs_locking_disable */
	return 0;
}

int wrs_locking_enable(struct pp_instance *ppi)
{
	/* FIXME wrs_locking_enable */
	return 0;
}

int wrs_locking_poll(struct pp_instance *ppi)
{
	/* FIXME wrs_locking_poll */
	return 0;
}

int wr_locking_enable(struct pp_instance *ppi)
	__attribute__((alias("wrs_locking_enable")));

int wr_locking_poll(struct pp_instance *ppi)
	__attribute__((alias("wrs_locking_poll")));

int wr_locking_disable(struct pp_instance *ppi)
	__attribute__((alias("wrs_locking_disable")));

int wr_enable_ptracker(struct pp_instance *ppi)
	__attribute__((alias("wrs_enable_ptracker")));

int wr_enable_timing_output(struct pp_instance *ppi, int enable)
	__attribute__((alias("wrs_enable_timing_output")));

int wr_adjust_in_progress()
	__attribute__((alias("wrs_adjust_in_progress")));

int wr_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec)
	__attribute__((alias("wrs_adjust_counters")));

int wr_adjust_phase(int32_t phase_ps)
	__attribute__((alias("wrs_adjust_phase")));

struct pp_time_operations wrs_time_ops = {
	.get = unix_time_get,
	.set = unix_time_set,
	.adjust = unix_time_adjust,
	.adjust_offset = unix_time_adjust_offset,
	.adjust_freq = unix_time_adjust_freq,
	.calc_timeout = unix_calc_timeout,
};
