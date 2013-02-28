/*
 * All code from uClibc-0.9.32. LGPL V2.1
 */
#include <ppsi/lib.h>

/* Architectures declare pp_puts: map puts (used by pp_printf) to it */
int puts(const char *s)
{
	pp_puts(s);
	return 0;
}

/* libc/string/strnlen.c */
size_t strnlen(const char *s, size_t max)
{
	register const char *p = s;

	while (max && *p) {
		++p;
		--max;
	}
	return p - s;
}

/* libc/string/memcpy.c */
void *memcpy(void *s1, const void *s2, size_t n)
{
	register char *r1 = s1;
	register const char *r2 = s2;

	while (n) {
		*r1++ = *r2++;
		--n;
	}
	return s1;
}

/* libc/string/memcmp.c */
int memcmp(const void *s1, const void *s2, size_t n)
{
	register const unsigned char *r1 = s1;
	register const unsigned char *r2 = s2;
	int r = 0;

	while (n-- && ((r = ((int)(*r1++)) - *r2++) == 0))
		;
	return r;
}

/* libc/string/memset.c */
void *memset(void *s, int c, size_t n)
{
	register unsigned char *p = s;

	while (n) {
		*p++ = (unsigned char) c;
		--n;
	}
	return s;
}


/* libc/string/strcpy.c */
char *strcpy(char *s1, const char *s2)
{
	register char *s = s1;

	while ( (*s++ = *s2++) != 0 )
		;
	return s1;
}

/* for strtol even uClibc is too heavy. Let's rewrite some hack */
long int strtol(const char *s, char **end, int base)
{
	long res = 0;

	if (!base)
		base = 10;
	while (*s) {
		if (end)
			*end = (void *)s;
		/* we only use it for cmdline: avoid alpha digits */
		if (base > 10 || *s < '0' || *s >= '0' + base) {
			return res;
		}
		res = res * base + *(s++) - '0';
	}
	return res;
}
int atoi(const char *s)
{
	return strtol(s, NULL, 10);
}
