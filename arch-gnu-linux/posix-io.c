/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#define _GNU_SOURCE /* for strnlen */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/timex.h>
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

const Integer32 PP_ADJ_FREQ_MAX = 512000;

static void clock_fatal_error(char *context)
{
	PP_PRINTF("failure in \"%s\": %s\n.Exiting.\n", context,
		  strerror(errno));
	exit(1);
}

void posix_puts(const char *s)
{
	fputs(s, stdout);
}

void posix_get_tstamp(TimeInternal *t)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0)
		clock_fatal_error("clock_gettime");
	t->seconds = tp.tv_sec;
	t->nanoseconds = tp.tv_nsec;
}

int32_t posix_set_tstamp(TimeInternal *t)
{
	struct timespec tp_orig;
	struct timespec tp;

	if (clock_gettime(CLOCK_REALTIME, &tp_orig) < 0)
		clock_fatal_error("clock_gettime");

	tp.tv_sec = t->seconds;
	tp.tv_nsec = t->nanoseconds;
	if (clock_settime(CLOCK_REALTIME, &tp) < 0)
		clock_fatal_error("clock_settime");

	return tp.tv_sec - tp_orig.tv_sec; /* handle only sec field, since
					    * timer granularity is 1s */
}

int posix_adj_freq(Integer32 adj)
{
	struct timex t;

	if (adj > PP_ADJ_FREQ_MAX)
		adj = PP_ADJ_FREQ_MAX;
	else if (adj < -PP_ADJ_FREQ_MAX)
		adj = -PP_ADJ_FREQ_MAX;

	t.modes = MOD_FREQUENCY;
	t.freq = adj * ((1 << 16) / 1000);

	return !adjtimex(&t);
}

void pp_puts(const char *s)
	__attribute__((alias("posix_puts")));

void pp_get_tstamp(TimeInternal *t)
	__attribute__((alias("posix_get_tstamp")));

int32_t pp_set_tstamp(TimeInternal *t)
	__attribute__((alias("posix_set_tstamp")));

int pp_adj_freq(Integer32 adj)
	__attribute__((alias("posix_adj_freq")));
