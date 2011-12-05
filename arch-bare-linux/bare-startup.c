/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */

/*
 * This is the startup thing for "freestanding" stuff under Linux.
 * It must also clear the BSS as I'm too lazy to do that in asm
 */
#include <pproto/pproto.h>
#include "bare-linux.h"

extern int __bss_start, __bss_end;

void pproto_clear_bss(void)
{
	int *ptr;

	for (ptr = &__bss_start; ptr < &__bss_end; ptr++)
		*ptr = 0;
}

static struct pp_instance ppi_static;

void pproto_main(void)
{
	struct pp_instance *ppi = &ppi_static; /* no malloc, one instance */

	pp_puts("bare: starting. Compiled on " __DATE__ "\n");

	if (bare_open_ch(ppi, "eth0")) {
		pp_diag_error(ppi, bare_errno);
		pp_diag_fatal(ppi, "open_ch", "");
	}
	pp_open_instance(ppi,0);

	bare_main_loop(ppi);
}
