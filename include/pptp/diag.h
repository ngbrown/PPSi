/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#ifndef __PPTP_DIAG_H__
#define __PPTP_DIAG_H__

#include <pptp/pptp.h>
/*
 * The diagnostic functions (diag-yes.c and diag-no.c).
 *
 * Use trace like "pp_diag_trace(ppi, __func__, __LINE__)".
 * error gets an integer, the other ones two strings (so we can
 * strerror(errno) together with the explanation. Avoid diag_printf if
 * possible, for size reasons, but here it is anyways.
 */
enum {
	STATE_ENTER,
	STATE_LEAVE
};

extern void pp_diag_fsm(struct pp_instance *ppi, char *name, int seq, int plen);
extern void pp_diag_trace(struct pp_instance *ppi, const char *f, int line);
extern void pp_diag_error(struct pp_instance *ppi, int err);
extern void pp_diag_error_str2(struct pp_instance *ppi, char *s1, char *s2);
extern void pp_diag_fatal(struct pp_instance *ppi, char *s1, char *s2);
extern void pp_diag_printf(struct pp_instance *ppi, char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

/* Our printf, that is implemented internally */
extern int pp_printf(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));
extern int pp_vprintf(const char *fmt, va_list args)
	__attribute__((format(printf, 1, 0)));
extern int pp_vsprintf(char *buf, const char *, va_list)
	__attribute__ ((format (printf, 2, 0)));

#endif /* __PPTP_DIAG_H__ */
