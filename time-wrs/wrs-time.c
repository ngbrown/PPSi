/*
 * Aurelio Colosimo for CERN, 2013 -- public domain
 */

#include <ppsi/ppsi.h>
#include <unix-time.h>

int wrs_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec)
{
	/* FIXME wrs_adjust_counters */
	return 0;
}

int wrs_adjust_phase(int32_t phase_ps)
{
	/* FIXME wrs_adjust_phase */
	return 0;
}

int wrs_adjust_in_progress(void)
{
	/* FIXME wrs_adjust_in_progress */
	return 0;
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
