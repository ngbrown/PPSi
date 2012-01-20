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

const Integer32 PP_ADJ_FREQ_MAX = 512000;

void posix_puts(const char *s)
{
	fputs(s, stdout);
}

int posix_strnlen(const char *s, int maxlen)
{
	return strnlen(s, maxlen);
}

void *posix_memcpy(void *d, const void *s, int count)
{
	return memcpy(d, s, count);
}

int posix_memcmp(const void *s1, const void *s2, int count)
{
	return memcmp(s1, s2, count);
}

void *posix_memset(void *s, int c, int count)
{
	return memset(s, c, count);
}

void posix_get_tstamp(TimeInternal *t)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
		/* FIXME diag PERROR("clock_gettime() failed, exiting."); */
		exit(0);
	}
	t->seconds = tp.tv_sec;
	t->nanoseconds = tp.tv_nsec;
}

int32_t posix_set_tstamp(TimeInternal *t)
{
	struct timespec tp_orig;
	struct timespec tp;

	if (clock_gettime(CLOCK_REALTIME, &tp_orig) < 0) {
		/* FIXME diag PERROR("clock_gettime() failed, exiting."); */
		exit(0);
	}

	tp.tv_sec = t->seconds;
	tp.tv_nsec = t->nanoseconds;
	if (clock_settime(CLOCK_REALTIME, &tp) < 0) {
		/* FIXME diag PERROR("clock_settime() failed, exiting."); */
		exit(0);
	}

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

int pp_strnlen(const char *s, int maxlen)
	__attribute__((alias("posix_strnlen")));

void *pp_memcpy(void *d, const void *s, int count)
	__attribute__((alias("posix_memcpy")));

int pp_memcmp(const void *s1, const void *s2, int count)
	__attribute__((alias("posix_memcmp")));

void *pp_memset(void *s, int c, int count)
	__attribute__((alias("posix_memset")));

void pp_get_tstamp(TimeInternal *t)
	__attribute__((alias("posix_get_tstamp")));

int32_t pp_set_tstamp(TimeInternal *t)
	__attribute__((alias("posix_set_tstamp")));

int pp_adj_freq(Integer32 adj)
	__attribute__((alias("posix_adj_freq")));
