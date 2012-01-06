/*
 * Alessandro Rubini and Aurelio Colosimo for CERN, 2011 -- public domain
 */
#ifndef __PTP_LIB_H__
#define __PTP_LIB_H__

/* We base on puts and a few more functions: each arch must have it */
extern void pp_puts(const char *s);
extern int pp_strnlen(const char *s, int maxlen);
extern void *pp_memcpy(void *d, const void *s, int count);
extern int pp_memcmp(const void *s1, const void *s2, int count);
extern void *pp_memset(void *s, int c, int count);

#endif /* __PTP_DEP_H__ */
