/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

/*
 * This has diagnostics. It calls pp_printf (which one, we don't know)
 */

void pp_diag_error(struct pp_instance *ppi, int err)
{
	pp_printf("ERR for %p: %i\n", ppi, err);
}

void pp_diag_error_str2(struct pp_instance *ppi, char *s1, char *s2)
{
	pp_printf("ERR for %p: %s %s\n", ppi, s1, s2);
}
