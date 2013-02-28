/*
 * Alessandro Rubini for CERN, 2013 -- LGPL 2.1 or later
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "bare-i386.h"

static int bare_time_get(TimeInternal *t)
{
	struct bare_timeval tv;

	if (sys_gettimeofday(&tv, NULL) < 0) {
		PP_PRINTF("gettimeofday error");
		sys_exit(1);
	}
	t->seconds = tv.tv_sec;
	t->nanoseconds = tv.tv_usec * 1000;
	if (pp_verbose_time)
		pp_printf("%s: %9li.%09li\n", __func__, tv.tv_sec, tv.tv_nsec);
	return 0;
}

static int bare_time_set(TimeInternal *t)
{
	struct bare_timeval tv;

	tv.tv_sec = t->seconds;
	tv.tv_usec = t->nanoseconds / 1000;

	if (sys_settimeofday(&tv, NULL) < 0) {
		PP_PRINTF("settimeofday error");
		sys_exit(1);
	}
	if (pp_verbose_time)
		pp_printf("%s: %9li.%09li\n", __func__, tv.tv_sec, tv.tv_nsec);
	return 0;
}

static int bare_time_adjust(long offset_ns, long freq_ppm)
{
	struct bare_timex t;
	int ret;

	if (freq_ppm > PP_ADJ_FREQ_MAX)
		freq_ppm = PP_ADJ_FREQ_MAX;
	if (freq_ppm < -PP_ADJ_FREQ_MAX)
		freq_ppm = -PP_ADJ_FREQ_MAX;

	t.offset = offset_ns / 1000;
	t.freq = freq_ppm; /* was: "adj * ((1 << 16) / 1000)" */
	t.modes = MOD_FREQUENCY | MOD_OFFSET;

	ret = sys_adjtimex(&t);
	if (pp_verbose_time)
		pp_printf("%s: %li %li\n", __func__, offset_ns, freq_ppm);
	return ret;
}

struct pp_time_operations pp_t_ops = {
	.get = bare_time_get,
	.set = bare_time_set,
	.adjust = bare_time_adjust,
};
