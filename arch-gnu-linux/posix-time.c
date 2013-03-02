/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/timex.h>
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

static void clock_fatal_error(char *context)
{
	PP_PRINTF("failure in \"%s\": %s\n.Exiting.\n", context,
		  strerror(errno));
	exit(1);
}

static int posix_time_get(TimeInternal *t)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0)
		clock_fatal_error("clock_gettime");
	t->seconds = tp.tv_sec;
	t->nanoseconds = tp.tv_nsec;
	if (pp_verbose_time)
		pp_printf("%s: %9li.%09li\n", __func__, tp.tv_sec, tp.tv_nsec);
	return 0;
}

int32_t posix_time_set(TimeInternal *t)
{
	struct timespec tp;

	tp.tv_sec = t->seconds;
	tp.tv_nsec = t->nanoseconds;
	if (clock_settime(CLOCK_REALTIME, &tp) < 0)
		clock_fatal_error("clock_settime");
	if (pp_verbose_time)
		pp_printf("%s: %9li.%09li\n", __func__, tp.tv_sec, tp.tv_nsec);
	return 0;
}

int posix_time_adjust(long offset_ns, long freq_ppm)
{
	struct timex t;
	int ret;

	if (freq_ppm > PP_ADJ_FREQ_MAX)
		freq_ppm = PP_ADJ_FREQ_MAX;
	if (freq_ppm < -PP_ADJ_FREQ_MAX)
		freq_ppm = -PP_ADJ_FREQ_MAX;

	t.offset = offset_ns / 1000;
	t.freq = freq_ppm; /* was: "adj * ((1 << 16) / 1000)" */
	t.modes = MOD_FREQUENCY | MOD_OFFSET;

	ret = adjtimex(&t);
	if (pp_verbose_time)
		pp_printf("%s: %li %li\n", __func__, offset_ns, freq_ppm);
	return ret;
}

static unsigned long posix_calc_timeout(int millisec)
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

struct pp_time_operations pp_t_ops = {
	.get = posix_time_get,
	.set = posix_time_set,
	.adjust = posix_time_adjust,
	.calc_timeout = posix_calc_timeout,
};
