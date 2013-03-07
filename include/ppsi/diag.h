/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#ifndef __PPSI_DIAG_H__
#define __PPSI_DIAG_H__

#include <ppsi/ppsi.h>

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
