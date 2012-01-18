/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#define _GNU_SOURCE /* for strnlen */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timex.h>
#include <pptp/pptp.h>

#define POSIX_IO_ADJ_FREQ_MAX	512000

void pp_puts(const char *s)
{
	fputs(s, stdout);
}

int pp_strnlen(const char *s, int maxlen)
{
	return strnlen(s, maxlen);
}

void *pp_memcpy(void *d, const void *s, int count)
{
	return memcpy(d, s, count);
}

int pp_memcmp(const void *s1, const void *s2, int count)
{
	return memcmp(s1, s2, count);
}

void *pp_memset(void *s, int c, int count)
{
	return memset(s, c, count);
}

void pp_get_tstamp(TimeInternal *t)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
		/* FIXME diag PERROR("clock_gettime() failed, exiting."); */
		exit(0);
	}
	t->seconds = tp.tv_sec;
	t->nanoseconds = tp.tv_nsec;
}

void pp_set_tstamp(TimeInternal *t)
{
	/* FIXME: what happens with timers? */
	struct timespec tp;
	tp.tv_sec = t->seconds;
	tp.tv_nsec = t->nanoseconds;
	if (clock_settime(CLOCK_REALTIME, &tp) < 0) {
		/* FIXME diag PERROR("clock_settime() failed, exiting."); */
		exit(0);
	}
}

int pp_adj_freq(Integer32 adj)
{
	struct timex t;

	if (adj > POSIX_IO_ADJ_FREQ_MAX)
		adj = POSIX_IO_ADJ_FREQ_MAX;
	else if (adj < -POSIX_IO_ADJ_FREQ_MAX)
		adj = -POSIX_IO_ADJ_FREQ_MAX;

	t.modes = MOD_FREQUENCY;
	t.freq = adj * ((1 << 16) / 1000);

	return !adjtimex(&t);
}
