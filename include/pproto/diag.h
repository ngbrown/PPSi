/*
 * FIXME
 */
#ifndef __PTP_DIAG_H__
#define __PTP_DIAG_H__

/*
 * The diagnostic functions (diag-yes.c and diag-no.c).
 *
 * Use trace like "pp_diag_trace(ppi, __func__, __LINE__)".
 * error gets an integer, the other ones two strings (so we can
 * strerror(errno) together with the explanation. Avoid diag_printf if
 * possible, for size reasons, but here it is anyways.
 */
extern void pp_diag_fsm(struct pp_instance *ppi, int sequence, int plen);
extern void pp_diag_trace(struct pp_instance *ppi, const char *f, int line);
extern void pp_diag_error(struct pp_instance *ppi, int err);
extern void pp_diag_error_str2(struct pp_instance *ppi, char *s1, char *s2);
extern void pp_diag_fatal(struct pp_instance *ppi, char *s1, char *s2);
extern void pp_diag_printf(struct pp_instance *ppi, char *fmt, ...)
	__attribute__((format(printf,2,3)));

#endif /* __PTP_DIAG_H__ */
