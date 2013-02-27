/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */

/*
 * This is the startup thing for "freestanding" stuff under Linux.
 * It must also clear the BSS as I'm too lazy to do that in asm
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "bare-i386.h"


void ppsi_clear_bss(void)
{
	int *ptr;
	extern int __bss_start, __bss_end;

	for (ptr = &__bss_start; ptr < &__bss_end; ptr++)
		*ptr = 0;
}

static struct pp_instance ppi_static;
static struct pp_net_path net_path_static;
CONST_VERBOSITY int pp_diag_verbosity = 0;

/* ppi fields */
static UInteger16 sent_seq_id[16];
static DSDefault defaultDS;
static DSCurrent currentDS;
static DSParent parentDS;
static DSPort portDS;
static DSTimeProperties timePropertiesDS;
static struct pp_servo servo;
static struct pp_frgn_master frgn_master;

void ppsi_main(void)
{
	struct pp_instance *ppi = &ppi_static; /* no malloc, one instance */

	PP_PRINTF("bare: starting. Compiled on %s\n", __DATE__);

	ppi->net_path = &net_path_static;
	ppi->sent_seq_id = sent_seq_id;
	ppi->defaultDS   = &defaultDS;
	ppi->currentDS   = &currentDS;
	ppi->parentDS    = &parentDS;
	ppi->portDS      = &portDS;
	ppi->timePropertiesDS = &timePropertiesDS;
	ppi->servo       = &servo;
	ppi->frgn_master = &frgn_master;
	ppi->arch_data   = NULL;

#if 0
	if (bare_open_ch(ppi, "eth0")) {
		pp_diag_error(ppi, bare_errno);
		pp_diag_fatal(ppi, "open_ch", "");
	}
#endif
	pp_open_instance(ppi, NULL);

	OPTS(ppi)->iface_name = "eth0";

	bare_main_loop(ppi);
}
