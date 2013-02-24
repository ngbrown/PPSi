/*
 * Alessandro Rubini for CERN, 2011 -- GPL 2 or later (it includes u-boot code)
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "bare-x86-64.h"

const Integer32 PP_ADJ_FREQ_MAX = 512000;

void pp_puts(const char *s)
{
	sys_write(0, s, strnlen(s, 300));
}

void bare_get_tstamp(TimeInternal *t)
{
	struct bare_timeval tv;

	if (sys_gettimeofday(&tv, NULL) < 0) {
		PP_PRINTF("gettimeofday error");
		sys_exit(0);
	}

	t->seconds = tv.tv_sec;
	t->nanoseconds = tv.tv_usec * 1000;
}

int32_t bare_set_tstamp(TimeInternal *t)
{
	struct bare_timeval tv_orig;
	struct bare_timeval tv;

	if (sys_gettimeofday(&tv_orig, NULL) < 0) {
		PP_PRINTF("gettimeofday error");
		sys_exit(0);
	}

	tv.tv_sec = t->seconds;
	tv.tv_usec = t->nanoseconds / 1000;

	if (sys_settimeofday(&tv, NULL) < 0) {
		PP_PRINTF("settimeofday error");
		sys_exit(0);
	}

	return tv.tv_sec - tv_orig.tv_sec;
}

int bare_adj_freq(Integer32 adj)
{
	struct bare_timex t;

	if (adj > PP_ADJ_FREQ_MAX)
		adj = PP_ADJ_FREQ_MAX;
	else if (adj < -PP_ADJ_FREQ_MAX)
		adj = -PP_ADJ_FREQ_MAX;

	t.modes = MOD_FREQUENCY;
	t.freq = adj * ((1 << 16) / 1000);

	return !sys_adjtimex(&t);
}

int32_t pp_set_tstamp(TimeInternal *t)
	__attribute__((alias("bare_set_tstamp")));
void pp_get_tstamp(TimeInternal *t)
	__attribute__((alias("bare_get_tstamp")));
int pp_adj_freq(Integer32 adj)
	__attribute__((alias("bare_adj_freq")));

