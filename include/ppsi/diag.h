/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#ifndef __PPSI_DIAG_H__
#define __PPSI_DIAG_H__

#include <ppsi/ppsi.h>

/* Our printf, that is implemented internally */
extern int pp_printf(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));
extern int pp_vprintf(const char *fmt, va_list args)
	__attribute__((format(printf, 1, 0)));
extern int pp_vsprintf(char *buf, const char *, va_list)
	__attribute__ ((format (printf, 2, 0)));

/*
 * The diagnostic functions
 *
 * error gets an integer, the other ones two strings (so we can
 * strerror(errno) together with the explanation. Avoid diag_printf if
 * possible, for size reasons, but here it is anyways.
 */
static inline void pp_diag_error(struct pp_instance *ppi, int err)
{
	pp_printf("ERR for %p: %i\n", ppi, err);
}

static inline void pp_diag_error_str2(struct pp_instance *ppi,
				      char *s1, char *s2)
{
	pp_printf("ERR for %p: %s %s\n", ppi, s1, s2);
}

#endif /* __PPSI_DIAG_H__ */
