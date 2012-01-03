/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <pptp/pptp.h>
#include <hw/wb_uart.h>
#include "spec.h"

void pp_puts(const char *s)
{
	spec_puts(s);
}

int pp_strnlen(const char *s, int maxlen)
{
	int len = 0;
	while (*(s++)) len++;
	return len;
}

void *pp_memcpy(void * dest, const void *src, int count)
{
	/* from u-boot-1.1.2 */
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

void pp_get_stamp(uint32_t *sptr)
{
	*sptr = htonl(spec_time());
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
