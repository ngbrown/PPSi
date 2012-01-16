/*
 * Alessandro Rubini for CERN, 2011 -- GPL 2 or later (it includes u-boot code)
 */
#include <pptp/pptp.h>
#include "bare-linux.h"

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

void pp_get_tstamp(TimeInternal *t)
{
	/* FIXME tstamp *sptr = htonl(sys_time(0));*/
}

int pp_memcmp(const void *cs, const void *ct, int count)
{
	/* from u-boot-1.1.2 */
	const unsigned char *su1, *su2;
	int res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
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
