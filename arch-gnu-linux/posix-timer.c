/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

/* Timer interface for GNU/Linux (and most likely other posix systems */
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "posix.h"

/* In ptpd-2.1.0/src/dep/timer.c the mechanism was different.
 * A SIGALARM timer was called once a second, increasing the counter of elapsed
 * time.
 * I find it easier to check for the timestamp when needed. The granularity of
 * this timer is the same as the ptpd solution: 1 second.
 * Should be checked if this is enough, but I guess yes.
 * Maybe a certain SIGALARM timer solution must be re-introduced, because the
 * select in the main loop must exit when a timer elapses. To be tested and
 * checked
 */

int posix_timer_init(struct pp_instance *ppi)
{
	struct pp_timer *t;
	int i;

	for (i = 0; i < PP_TIMER_ARRAY_SIZE; i++) {

		t = calloc(1, sizeof(*t));
		if (!t)
			return -1;

		ppi->timers[i] = t;
	}

	return 0;
}


int posix_timer_start(uint32_t interval_ms, struct pp_timer *tm)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	tm->start = ((uint64_t)(now.tv_sec)) * 1000 +
			(now.tv_nsec / 1000000);
	tm->interval_ms = interval_ms;

	return 0;
}

int posix_timer_stop(struct pp_timer *tm)
{
	tm->interval_ms = 0;
	tm->start = 0;
	return 0;
}

int posix_timer_expired(struct pp_timer *tm)
{
	struct timespec now;
	uint64_t now_ms;

	if (tm->start == 0) {
		PP_PRINTF("%s: Warning: timer %p not started\n", __func__, tm);
		return 0;
	}

	clock_gettime(CLOCK_MONOTONIC, &now);

	now_ms = ((uint64_t)(now.tv_sec)) * 1000 +
			(now.tv_nsec / 1000000);

	if (now_ms > tm->start + tm->interval_ms) {
		tm->start = now_ms;
		return 1;
	}

	return 0;
}

int pp_timer_init(struct pp_instance *ppi)
	__attribute__((alias("posix_timer_init")));

int pp_timer_start(uint32_t interval_ms, struct pp_timer *tm)
	__attribute__((alias("posix_timer_start")));

int pp_timer_stop(struct pp_timer *tm)
	__attribute__((alias("posix_timer_stop")));

int pp_timer_expired(struct pp_timer *tm)
	__attribute__((alias("posix_timer_expired")));
