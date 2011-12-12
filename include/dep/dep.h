/*
 * FIXME: header
 */
#ifndef __PTP_DEP_H__
#define __PTP_DEP_H__

/* Our printf, that is implemented internally */
extern int pp_printf(const char *fmt, ...)
	__attribute__((format(printf,1,2)));
extern int pp_vsprintf(char *buf, const char *, va_list)
        __attribute__ ((format (printf, 2, 0)));

/* We base on puts and a few more functions: each arch must have it */
extern void pp_puts(const char *s);
extern int pp_strnlen(const char *s, int maxlen);
extern void *pp_memcpy(void *d, const void *s, int count);
extern int pp_memcmp(void *s1, const void *s2, int count);
extern void *pp_memset(void *s, int c, int count);

#endif /* __PTP_DEP_H__ */
