/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>
#include <hw/wb_uart.h>
#include "spec.h"

const Integer32 PP_ADJ_FREQ_MAX = 512000; //GGDD value ?

void pp_puts(const char *s)
{
	spec_puts(s);
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

void pp_get_tstamp(TimeInternal *t) //uint32_t *sptr)
{
	pps_gen_get_time(&t->seconds, &t->nanoseconds );
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

void *pp_memset(void *s, int c, int count)
{
	return memset(s, c, count);
}

void *memset(void *s, int c, int count)
{
	/* from u-boot-1.1.2 */
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

void *memcpy(void *dest, const void *src, int count)
{
	/* from u-boot-1.1.2 */
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

int pp_memcmp(const void *s1, const void *s2, int count)
{
	unsigned char *u1 = (unsigned char*) s1;
	unsigned char *u2 = (unsigned char*) s2;
	while (count-- != 0) 
	{
		if (*u1 != *u2)
			return (*u1 < *u2) ? -1 : +1;
		u1++;
		u2++;
	}
	return 0;
}

int32_t spec_set_tstamp(TimeInternal *t)
{
	TimeInternal tp_orig;

	pps_gen_get_time( &tp_orig.seconds, &tp_orig.nanoseconds );
	pps_gen_adjust_utc(t->seconds);
	pps_gen_adjust_nsec(t->nanoseconds);

	return t->seconds - tp_orig.seconds; /* handle only sec field, since
					    * timer granularity is 1s */
}

int spec_adj_freq(Integer32 adj)
{
	//GGDD
	return 0;
}

int pp_adj_freq(Integer32 adj)
	__attribute__((alias("spec_adj_freq")));


int32_t pp_set_tstamp(TimeInternal *t)
	__attribute__((alias("spec_set_tstamp")));
