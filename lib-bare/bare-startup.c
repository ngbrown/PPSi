/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
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

/* ppg fields */
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
	struct bare_timex t;

	pp_printf("ppsi starting. Built on %s\n", __DATE__);

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

	ppg->rt_opts = &default_rt_opts;

	if (sys_adjtimex(&t) >= 0)
		timePropertiesDS.currentUtcOffset = t.tai;

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	/* This just allocates the stuff */
	pp_open_globals(ppg);

	/* The actual sockets are opened in state-initializing */
	bare_main_loop(ppi);
	return 0;
}

/* We can't parse a config file or string (no system calls, /me is lazy) */
int pp_config_file(struct pp_globals *ppg, int force, char *fname)
{
	pp_printf("No support for config file: can't read \"%s\"\n",
		  fname);
	return -1;
}

int pp_config_string(struct pp_globals *ppg, char *s)
{
	pp_printf("No support for config options: can't parse \"%s\"\n", s);
	return -1;
}
