/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>

/*
 * Having no diagnostics, just make one function that returns 0 and alias
 * all the rest to it (those returning void will have the caller ignore our 0)
 */
int pp_diag_nop(void)
{
	return 0;
}

void pp_diag_fsm(struct pp_instance *ppi, int sequence, int plen)
	__attribute__((weak,alias("pp_diag_nop")));

void pp_diag_trace(struct pp_instance *ppi, const char *f, int line)
	__attribute__((weak,alias("pp_diag_nop")));

void pp_diag_error(struct pp_instance *ppi, int err)
	__attribute__((weak,alias("pp_diag_nop")));

void pp_diag_error_str2(struct pp_instance *ppi, char *s1, char *s2)
	__attribute__((weak,alias("pp_diag_nop")));

void pp_diag_fatal(struct pp_instance *ppi, char *s1, char *s2)
	__attribute__((weak,alias("pp_diag_nop")));

void pp_diag_printf(struct pp_instance *ppi, char *fmt, ...)
	__attribute__((weak,alias("pp_diag_nop")));

int pp_printf(const char *fmt, ...)
	__attribute__((weak,alias("pp_diag_nop")));

int pp_vprintf(const char *fmt, va_list args)
	__attribute__((weak,alias("pp_diag_nop")));
