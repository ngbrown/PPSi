/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#define _GNU_SOURCE /* for strnlen */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pproto/pproto.h>

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

void pp_get_stamp(uint32_t *sptr)
{
	*sptr = htonl(time(NULL));
}
