/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Pietro Fezzardi (pietrofezzardi@gmail.com)
 *
 * Released to the public domain
 */

/*
 * Time operations for the simulator, design to be used both for
 * master and the slave instance.
 */

#include <time.h>
#include <errno.h>
#include <ppsi/ppsi.h>
#include "../arch-sim/ppsi-sim.h"

static int sim_time_get(struct pp_instance *ppi, TimeInternal *t)
{
	struct timespec ts;

	ts.tv_nsec = SIM_PPI_ARCH(ppi)->time.current_ns %
				(long long)PP_NSEC_PER_SEC;
	ts.tv_sec = (SIM_PPI_ARCH(ppi)->time.current_ns - ts.tv_nsec) /
				(long long)PP_NSEC_PER_SEC;

	t->nanoseconds = ts.tv_nsec;
	t->seconds = ts.tv_sec;
	t->correct = 1;

	if (!(pp_global_flags & PP_FLAG_NOTIMELOG))
		pp_diag(ppi, time, 2, "%s: %9li.%09li\n", __func__,
			(long)t->seconds, (long)t->nanoseconds);
	return 0;
}

static int sim_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	if (!t) {
		/* Change the network notion of utc/tai offset */
		return 0;
	}

	SIM_PPI_ARCH(ppi)->time.current_ns = t->nanoseconds
				+ t->seconds * (long long)PP_NSEC_PER_SEC;

	pp_diag(ppi, time, 1, "%s: %9i.%09i\n", __func__,
			t->seconds, t->nanoseconds);
	return 0;
}

static int sim_time_adjust(struct pp_instance *ppi, long offset_ns,
				long freq_ppm)
{
	if (freq_ppm) {
		if (freq_ppm > PP_ADJ_FREQ_MAX)
			freq_ppm = PP_ADJ_FREQ_MAX;
		if (freq_ppm < -PP_ADJ_FREQ_MAX)
			freq_ppm = -PP_ADJ_FREQ_MAX;
		SIM_PPI_ARCH(ppi)->time.freq_ppm_servo = freq_ppm;
	}

	if (offset_ns)
		SIM_PPI_ARCH(ppi)->time.current_ns += offset_ns;

	pp_diag(ppi, time, 1, "%s: %li %li\n", __func__, offset_ns, freq_ppm);
	return 0;
}

static int sim_adjust_freq(struct pp_instance *ppi, long freq_ppm)
{
	return sim_time_adjust(ppi, 0, freq_ppm);
}

static int sim_adjust_offset(struct pp_instance *ppi, long offset_ns)
{
	return sim_time_adjust(ppi, offset_ns, 0);
}

static inline int sim_init_servo(struct pp_instance *ppi)
{
	return SIM_PPI_ARCH(ppi)->time.freq_ppm_servo;
}

static unsigned long sim_calc_timeout(struct pp_instance *ppi, int millisec)
{
	unsigned long res;

	res = millisec + SIM_PPI_ARCH(ppi)->time.current_ns / 1000LL / 1000LL;
	return res ? res : 1;
}

struct pp_time_operations sim_time_ops = {
	.get = sim_time_get,
	.set = sim_time_set,
	.adjust = sim_time_adjust,
	.adjust_offset = sim_adjust_offset,
	.adjust_freq = sim_adjust_freq,
	.init_servo = sim_init_servo,
	.calc_timeout = sim_calc_timeout,
};
