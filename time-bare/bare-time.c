/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include "bare-linux.h"

static int bare_time_get(struct pp_instance *ppi, TimeInternal *t)
{
	struct bare_timeval tv;

	if (sys_gettimeofday(&tv, NULL) < 0) {
		pp_error("%s:", __func__);
		sys_exit(1);
	}
	/* TAI = UTC + 35 */
	t->seconds = tv.tv_sec + DSPRO(ppi)->currentUtcOffset;
	t->nanoseconds = tv.tv_usec * 1000;
	if (!(pp_global_d_flags & PP_FLAG_NOTIMELOG))
		pp_diag(ppi, time, 2, "%s: %9li.%06li\n", __func__,
			tv.tv_sec, tv.tv_usec);
	return 0;
}

static int bare_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	struct bare_timeval tv;

	if (!t) { /* Change the network notion of the utc/tai offset */
		struct bare_timex t;

		t.modes = MOD_TAI;
		t.constant = DSPRO(ppi)->currentUtcOffset;
		if (sys_adjtimex(&t) < 0)
			pp_diag(ppi, time, 1, "Can't change TAI offset");
		else
			pp_diag(ppi, time, 1, "New TAI offset: %i\n",
				DSPRO(ppi)->currentUtcOffset);
		return 0;
	}

	/* UTC = TAI - 35 */
	tv.tv_sec = t->seconds - DSPRO(ppi)->currentUtcOffset;
	tv.tv_usec = t->nanoseconds / 1000;

	if (sys_settimeofday(&tv, NULL) < 0) {
		pp_error("%s:", __func__);
		sys_exit(1);
	}
	pp_diag(ppi, time, 1, "%s: %9li.%06li\n", __func__, tv.tv_sec,
		tv.tv_usec);
	return 0;
}

static int bare_time_adjust(struct pp_instance *ppi, long offset_ns,
			    long freq_ppb)
{
	struct bare_timex t;
	int ret;

	/* FIXME: handle MOD_FREQUENCY and MOD_OFFSET separately, and
	 * set t.modes only for the mode having non-zero parameter.
	 * See unix_time_adjust in arch-unix/unix-time.c */
	if (freq_ppb > PP_ADJ_FREQ_MAX)
		freq_ppb = PP_ADJ_FREQ_MAX;
	if (freq_ppb < -PP_ADJ_FREQ_MAX)
		freq_ppb = -PP_ADJ_FREQ_MAX;

	t.offset = offset_ns / 1000;
	t.freq = freq_ppb * ((1 << 16) / 1000);
	t.modes = MOD_FREQUENCY | MOD_OFFSET;

	ret = sys_adjtimex(&t);
	pp_diag(ppi, time, 1, "%s: %li %li\n", __func__, offset_ns, freq_ppb);
	return ret;
}

int bare_time_adjust_offset(struct pp_instance *ppi, long offset_ns)
{
	return bare_time_adjust(ppi, offset_ns, 0);
}

int bare_time_adjust_freq(struct pp_instance *ppi, long freq_ppb)
{
	return bare_time_adjust(ppi, 0, freq_ppb);
}

static unsigned long bare_calc_timeout(struct pp_instance *ppi, int millisec)
{
	struct bare_timespec now;
	uint64_t now_ms;
	unsigned long result;

	if (!millisec)
		millisec = 1;
	sys_clock_gettime(CLOCK_MONOTONIC, &now);
	now_ms = 1000LL * now.tv_sec + now.tv_nsec / 1000 / 1000;

	result = now_ms + millisec;
	return result ? result : 1; /* cannot return 0 */
}

struct pp_time_operations bare_time_ops = {
	.get = bare_time_get,
	.set = bare_time_set,
	.adjust = bare_time_adjust,
	.adjust_offset = bare_time_adjust_offset,
	.adjust_freq = bare_time_adjust_freq,
	.calc_timeout = bare_calc_timeout,
};
