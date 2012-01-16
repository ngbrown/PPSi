/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

/* Timer interface for GNU/Linux (and most likely other posix systems */
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <pptp/pptp.h>
#include <pptp/diag.h>
#include "posix.h"

/* FIXME
 * In ptpd-2.1.0/src/dep/timer.c the mechanism was different.
 * A SIGALARM timer was called once a second, increasing the counter of elapsed
 * time.
 * I find it easier to check for the timestamp when needed. The granularity of
 * this timer is the same as the ptpd solution: 1 second.
 * Should be checked if this is enough, but I guess yes.
 * Maybe a certain SIGALARM timer solution must be re-introduced, because the
 * select in the main loop must exit when a timer elapses. To be checked
 */

extern int posix_timer_init(struct pp_instance *ppi)
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


extern int posix_timer_start(uint32_t interval, struct pp_timer *tm)
{
	time((time_t*)&tm->start);
	tm->interval = interval;

	return 0;
}

extern int posix_timer_stop(struct pp_timer *tm)
{
	tm->interval = 0;
	tm->start = 0;

	return 0;
}

extern int posix_timer_expired(struct pp_timer *tm)
{
	uint32_t now;

	if (tm->start == 0) {
		/* FIXME: print a warning message */
		return 0;
	}

	time((time_t*)&now);

	if (tm->start + tm->interval < now)
		return 1;

	return 0;
}


int pp_timer_init(struct pp_instance *ppi)
	__attribute__((alias("posix_timer_init")));

int pp_timer_start(uint32_t interval, struct pp_timer *tm)
	__attribute__((alias("posix_timer_start")));

int pp_timer_stop(struct pp_timer *tm)
	__attribute__((alias("posix_timer_stop")));

int pp_timer_expired(struct pp_timer *tm)
	__attribute__((alias("posix_timer_expired")));
