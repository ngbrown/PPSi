/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */

/*
 * This is the startup thing for "freestanding" stuff under Linux.
 * It must also clear the BSS as I'm too lazy to do that in asm
 */
#include <ppsi/ppsi.h>
#include "bare-linux.h"


void ppsi_clear_bss(void)
{
	int *ptr;
	extern int __bss_start, __bss_end;

	for (ptr = &__bss_start; ptr < &__bss_end; ptr++)
		*ptr = 0;
}

static struct pp_globals ppg_static;
static struct pp_instance ppi_static;
CONST_VERBOSITY int pp_diag_verbosity = 0;

/* ppi fields */
static DSDefault defaultDS;
static DSCurrent currentDS;
static DSParent parentDS;
static DSPort portDS;
static DSTimeProperties timePropertiesDS;
static struct pp_servo servo;

int ppsi_main(int argc, char **argv)
{
	struct pp_globals *ppg = &ppg_static;
	struct pp_instance *ppi = &ppi_static; /* no malloc, one instance */

	PP_PRINTF("bare: starting. Compiled on %s\n", __DATE__);

	ppi->glbs        = ppg;
	ppg->defaultDS   = &defaultDS;
	ppg->currentDS   = &currentDS;
	ppg->parentDS    = &parentDS;
	ppi->portDS      = &portDS;
	ppg->timePropertiesDS = &timePropertiesDS;
	ppg->servo = &servo;
	ppg->arch_data   = NULL;
	ppi->n_ops       = &bare_net_ops;
	ppi->t_ops       = &bare_time_ops;

	ppi->ethernet_mode = PP_DEFAULT_ETHERNET_MODE;
	ppi->iface_name = "eth0";

	/* This just llocates the stuff */
	pp_open_globals(ppg, NULL);

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	/* The actual sockets are opened in state-initializing */
	bare_main_loop(ppi);
	return 0;
}
