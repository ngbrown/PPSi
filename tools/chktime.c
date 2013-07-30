/*
 * Copyright (C) 2012 Alessandro Rubini
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>

static int64_t get_diff(struct timespec *ts1p)
{
	struct sched_param prio;
	struct timespec ts2, ts3;
	int64_t d1, d2, d3;

	prio.sched_priority = 1;
	if (sched_setscheduler(0, SCHED_FIFO, &prio)<0) {
		//fprintf(stderr, "setscheduler: %s\n", strerror(errno));
	}

	/* return the monotonic time with the pointer, and the diff as retval */
	clock_gettime(CLOCK_REALTIME, &ts2);
	clock_gettime(CLOCK_MONOTONIC, ts1p);
	clock_gettime(CLOCK_REALTIME, &ts3);

	if (sched_setscheduler(0, SCHED_OTHER, NULL)<0) {
		//fprintf(stderr, "setscheduler: %s\n", strerror(errno));
	}

	d1 = ts1p->tv_sec * 1000LL * 1000 * 1000 + ts1p->tv_nsec;
	d2 = ts2.tv_sec   * 1000LL * 1000 * 1000 + ts2.tv_nsec;
	d3 = ts3.tv_sec   * 1000LL * 1000 * 1000 + ts3.tv_nsec;

	d2 +=  (d3 - d2) / 2;

	return d2 - d1;

}

int main(int argc, char **argv)
{
	int64_t now, diff0, diff, newdiff, prevprint = 0;
	int64_t nextloop;
	struct timespec ts;
	int msecs = 10;

	if (argc > 1) {
		msecs = atoi(argv[1]);
		if (!msecs)
			msecs = 10;
	}
	fprintf(stderr, "%s: looping every %i millisecs\n", argv[0], msecs);

	diff0 = get_diff(&ts);
	nextloop = ts.tv_sec * 1000LL * 1000 + ts.tv_nsec / 1000; /* usecs */

	while (1) {
		diff = get_diff(&ts);
		newdiff = diff - diff0;

		if (newdiff > prevprint + 500 * 1000
		    || newdiff < prevprint - 500 * 1000) {
			/*
			 * The new difference is more than 0.5ms away
			 * from the value we printed previously: print it
			 */
			struct timeval tv;
			struct tm tm;
			char s[32];

			gettimeofday(&tv, NULL);
			localtime_r(&tv.tv_sec, &tm);
			strftime(s, sizeof(s), "%y-%m-%d-%H:%M:%S", &tm);
			printf("%s: %10lli us\n", s, (long long)newdiff / 1000);
			prevprint = newdiff;
			fflush(stdout); /* for our outgoing pipe */
		}

		/* wait up to next event, using the ts value we have already */
		nextloop += msecs * 1000;
		now =  ts.tv_sec * 1000LL * 1000 + ts.tv_nsec / 1000;
		//printf("%lli -> %lli\n", now, nextloop);
		if (now < nextloop)
			usleep(nextloop-now);
	}
}
