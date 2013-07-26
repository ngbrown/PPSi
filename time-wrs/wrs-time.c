/*
 * Aurelio Colosimo for CERN, 2013 -- public domain
 */

#include <errno.h>
#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>

#include <hal_exports.h>
/* FIXME: these externs are needed here because we can not include
 * hal_exports.h with HAL_EXPORT_STRUCTURES twice (the first is by
 * arch-wrs/wrs-calibration.c): structs are declared and
 * defined in .h file, so this would lead to a multiple definition. */
extern struct minipc_pd __rpcdef_pps_cmd;
extern struct minipc_pd __rpcdef_lock_cmd;

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
		&rval, HEXP_PPSG_CMD_POLL, &p);

	if ((ret < 0) || rval)
		return 0;

	return 1;
}

int wrs_enable_ptracker(struct pp_instance *ppi)
{
	int ret, rval;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_lock_cmd,
			  &rval, ppi->iface_name, HEXP_LOCK_CMD_ENABLE_TRACKING, 0);

	if ((ret < 0) || (rval < 0))
		return WR_SPLL_ERROR;

	return WR_SPLL_OK;
}

int wrs_enable_timing_output(struct pp_instance *ppi, int enable)
{
	int ret, rval;

	hexp_pps_params_t p;

	p.pps_valid = enable;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd,
			&rval, HEXP_PPSG_CMD_SET_VALID, &p);

	if ((ret < 0) || (rval < 0))
		return WR_SPLL_ERROR;

	return WR_SPLL_OK;
}

int wrs_locking_disable(struct pp_instance *ppi)
{
	return WR_SPLL_OK;
}

int wrs_locking_enable(struct pp_instance *ppi)
{
	int ret, rval;

	pp_diag(ppi, time, 1, "Start locking\n");

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_lock_cmd,
			  &rval, ppi->iface_name, HEXP_LOCK_CMD_START, 0);

	if ((ret < 0) || (rval < 0))
		return WR_SPLL_ERROR;

	return WR_SPLL_OK;
}

int wrs_locking_poll(struct pp_instance *ppi)
{
	int ret, rval;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_lock_cmd,
			  &rval, ppi->iface_name, HEXP_LOCK_CMD_CHECK, 0);

	if (ret != HEXP_LOCK_STATUS_LOCKED)
		return WR_SPLL_ERROR; /* FIXME should be WR_SPLL_NOT_READY */

	return WR_SPLL_READY;
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

static int wrs_time_get(struct pp_instance *ppi, TimeInternal *t)
{

	hexp_pps_params_t p;
	int cmd;
	int ret, rval;

	cmd = HEXP_PPSG_CMD_GET;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd, &rval,
					  cmd, &p);

	/* FIXME Don't know whether p.current_phase_shift is to be assigned
	 * to t->phase or t->raw_phase. I ignore it, it's not useful here. */
	t->seconds = p.current_sec;
	t->nanoseconds = p.current_nsec;
	t->correct = p.pps_valid;

	if (ret < 0)
		return ret;

	return rval;
}

static int32_t wrs_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	/* FIXME Don't know how to implement this. Actually it's almost unused
	 * in ppsi, only proto-standard/servo.c calls it, at initialization
	 * time */

	return -ENOTSUP;
}

static int wrs_time_adjust_offset(struct pp_instance *ppi, long offset_ns)
{
	return wr_adjust_counters(0, offset_ns);
}

static int wrs_time_adjust(struct pp_instance *ppi, long offset_ns,
			   long freq_ppm)
{
	if (freq_ppm != 0)
		pp_diag(ppi, time, 1, "Warning: %s: can not adjust freq_ppm %li\n",
				__func__, freq_ppm);
	return wrs_time_adjust_offset(ppi, offset_ns);
}

static unsigned long wrs_calc_timeout(struct pp_instance *ppi,
				      int millisec)
{
	/* We can rely on unix's CLOCK_MONOTONIC timing for timeouts */
	return unix_time_ops.calc_timeout(ppi, millisec);
}

struct pp_time_operations wrs_time_ops = {
	.get = wrs_time_get,
	.set = wrs_time_set,
	.adjust = wrs_time_adjust,
	.adjust_offset = wrs_time_adjust_offset,
	.adjust_freq = NULL,
	.calc_timeout = wrs_calc_timeout,
};
