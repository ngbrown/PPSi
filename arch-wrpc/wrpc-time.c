/*
 * Alessandro Rubini for CERN, 2013 -- LGPL 2.1 or later
 * (Mostly code by Tomasz Wlostowski in wrpc-sw)
 */
#include <ppsi/ppsi.h>
#include "wrpc.h"
#include "pps_gen.h" /* in wrpc-sw */
#include "syscon.h" /* in wrpc-sw */

static int wrpc_time_get(struct pp_instance *ppi, TimeInternal *t)
{
	uint64_t sec;
	uint32_t nsec;

	shw_pps_gen_get_time(&sec, &nsec);

	t->seconds = sec;
	t->nanoseconds = nsec;
	if (!(pp_global_flags & PP_FLAG_NOTIMELOG))
		pp_diag(ppi, time, 2, "%s: %9lu.%09li\n", __func__,
			(long)sec, (long)nsec);
	return 0;
}

static int wrpc_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	uint64_t sec;
	unsigned long nsec;

	sec = t->seconds;
	nsec = t->nanoseconds;

	shw_pps_gen_set_time(sec, nsec);
	pp_diag(ppi, time, 1, "%s: %9lu.%09li\n", __func__,
		(long)sec, (long)nsec);
	return 0;
}

static int wrpc_time_adjust(struct pp_instance *ppi, long offset_ns,
			    long freq_ppm)
{
	pp_diag(ppi, time, 1, "%s: %li %li\n",
		__func__, offset_ns, freq_ppm);
	if (offset_ns)
		shw_pps_gen_adjust(PPSG_ADJUST_NSEC, offset_ns);
	return 0;
}

static unsigned long wrpc_calc_timeout(struct pp_instance *ppi, int millisec)
{
	unsigned long now_ms = timer_get_tics();

	now_ms += millisec;
	return now_ms ? now_ms : 1; /* cannot return 0 */
}

struct pp_time_operations wrpc_time_ops = {
	.get = wrpc_time_get,
	.set = wrpc_time_set,
	.adjust = wrpc_time_adjust,
	.calc_timeout = wrpc_calc_timeout,
};
