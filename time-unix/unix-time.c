/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/timex.h>
#include <ppsi/ppsi.h>

static void clock_fatal_error(char *context)
{
	pp_error("failure in \"%s\": %s\n.Exiting.\n", context,
		  strerror(errno));
	exit(1);
}

static int unix_time_get(struct pp_instance *ppi, TimeInternal *t)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0)
		clock_fatal_error("clock_gettime");
	t->seconds = tp.tv_sec;
	t->nanoseconds = tp.tv_nsec;
	if (!(pp_global_flags & PP_FLAG_NOTIMELOG))
		pp_diag(ppi, time, 2, "%s: %9li.%09li\n", __func__,
			tp.tv_sec, tp.tv_nsec);
	return 0;
}

static int32_t unix_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	struct timespec tp;

	tp.tv_sec = t->seconds;
	tp.tv_nsec = t->nanoseconds;
	if (clock_settime(CLOCK_REALTIME, &tp) < 0)
		clock_fatal_error("clock_settime");
	pp_diag(ppi, time, 1, "%s: %9li.%09li\n", __func__,
		tp.tv_sec, tp.tv_nsec);
	return 0;
}

static int unix_time_adjust(struct pp_instance *ppi, long offset_ns, long freq_ppm)
{
	struct timex t;
	int ret;

	t.modes = 0;

	if (freq_ppm) {
		if (freq_ppm > PP_ADJ_FREQ_MAX)
			freq_ppm = PP_ADJ_FREQ_MAX;
		if (freq_ppm < -PP_ADJ_FREQ_MAX)
			freq_ppm = -PP_ADJ_FREQ_MAX;

		t.freq = freq_ppm * ((1 << 16) / 1000);
		t.modes = MOD_FREQUENCY;
	}

	if (offset_ns) {
		t.offset = offset_ns / 1000;
		t.modes |= MOD_OFFSET;
	}

	ret = adjtimex(&t);
	pp_diag(ppi, time, 1, "%s: %li %li\n", __func__, offset_ns, freq_ppm);
	return ret;
}

static int unix_time_adjust_offset(struct pp_instance *ppi, long offset_ns)
{
	return unix_time_adjust(ppi, offset_ns, 0);
}

static int unix_time_adjust_freq(struct pp_instance *ppi, long freq_ppm)
{
	return unix_time_adjust(ppi, 0, freq_ppm);
}

static unsigned long unix_calc_timeout(struct pp_instance *ppi, int millisec)
{
	struct timespec now;
	uint64_t now_ms;
	unsigned long result;

	if (!millisec)
		millisec = 1;
	clock_gettime(CLOCK_MONOTONIC, &now);
	now_ms = 1000LL * now.tv_sec + now.tv_nsec / 1000 / 1000;

	result = now_ms + millisec;
	return result ? result : 1; /* cannot return 0 */
}

struct pp_time_operations unix_time_ops = {
	.get = unix_time_get,
	.set = unix_time_set,
	.adjust = unix_time_adjust,
	.adjust_offset = unix_time_adjust_offset,
	.adjust_freq = unix_time_adjust_freq,
	.calc_timeout = unix_calc_timeout,
};
