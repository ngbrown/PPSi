/*
 * Alessandro Rubini for CERN, 2011 -- GPL 2 or later (it includes u-boot code)
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "bare-i386.h"

const Integer32 PP_ADJ_FREQ_MAX = 512000;

void pp_puts(const char *s)
{
	sys_write(0, s, pp_strnlen(s, 300));
}

int pp_strnlen(const char *s, int maxlen)
{
	int len = 0;
	while (*(s++))
		len++;
	return len;
}

void *pp_memcpy(void *dest, const void *src, int count)
{
	/* from u-boot-1.1.2 */
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
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

int pp_memcmp(const void *cs, const void *ct, int count)
{
	/* from u-boot-1.1.2 */
	const unsigned char *su1, *su2;
	int res = 0;

	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}

void *pp_memset(void *s, int c, int count)
{
	/* from u-boot-1.1.2 */
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

/* What follows has no prefix because it's only used by arch code */
char *strcpy(char *dest, const char *src)
{
	/* from u-boot-1.1.2 */
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}

void *memset(void *s, int c, int count)
	__attribute__((alias("pp_memset")));
void *memcpy(void *dest, const void *src, int count)
	__attribute__((alias("pp_memcpy")));

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

