/*
 * Alessandro Rubini for CERN, 2011 -- GPL 2 or later (it includes u-boot code)
 * (FIXME: turn to LGPL by using uclibc or equivalent -- avoid rewriting :)
 */
#include <ppsi/lib.h>

int strnlen(const char *s, int maxlen)
{
	int len = 0;
	while (*(s++))
		len++;
	return len;
}

void *memcpy(void *dest, const void *src, int count)
{
	/* from u-boot-1.1.2 */
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

int memcmp(const void *cs, const void *ct, int count)
{
	/* from u-boot-1.1.2 */
	const unsigned char *su1, *su2;
	int res = 0;

	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}

void *memset(void *s, int c, int count)
{
	/* from u-boot-1.1.2 */
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

char *strcpy(char *dest, const char *src)
{
	/* from u-boot-1.1.2 */
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}

/* As a first step in removing pp_memset etc, provide aliases */
int pp_strnlen(const char *s, int maxlen)
	__attribute__((alias("strnlen")));
extern char *pp_strcpy(char *dest, const char *src)
	__attribute__((alias("strcpy")));
extern void *pp_memcpy(void *d, const void *s, int count)
	__attribute__((alias("memcpy")));
extern int pp_memcmp(const void *s1, const void *s2, int count)
	__attribute__((alias("memcmp")));
extern void *pp_memset(void *s, int c, int count)
	__attribute__((alias("memset")));
