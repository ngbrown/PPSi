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

/* ppg fields */
static DSDefault defaultDS;
static DSCurrent currentDS;
static DSParent parentDS;
static DSPort portDS;
static DSTimeProperties timePropertiesDS;
static struct pp_servo servo;

static struct pp_globals ppg_static; /* forward declaration */
static unsigned char __tx_buffer[PP_MAX_FRAME_LENGTH];
static unsigned char __rx_buffer[PP_MAX_FRAME_LENGTH];

static struct pp_instance ppi_static = {
	.glbs			= &ppg_static,
	.portDS			= &portDS,
	.n_ops			= &bare_net_ops,
	.t_ops			= &bare_time_ops,
	.iface_name 		= "eth0",
	.port_name 		= "eth0",
	.proto			= PP_DEFAULT_PROTO,
	.__tx_buffer		= __tx_buffer,
	.__rx_buffer		= __rx_buffer,
};

/* We now have a structure with all globals, and multiple ppi inside */
static struct pp_globals ppg_static = {
	.pp_instances		= &ppi_static,
	.nlinks			= 1,
	.servo			= &servo,
	.defaultDS		= &defaultDS,
	.currentDS		= &currentDS,
	.parentDS		= &parentDS,
	.timePropertiesDS	= &timePropertiesDS,
	.rt_opts		= &__pp_default_rt_opts,
};

int ppsi_main(int argc, char **argv)
{
	struct pp_globals *ppg = &ppg_static;
	struct pp_instance *ppi = &ppi_static; /* no malloc, one instance */
	struct bare_timex t;

	pp_printf("PPSi, bare kernel. Commit %s, built on " __DATE__ "\n",
		PPSI_VERSION);

	ppg->rt_opts = &__pp_default_rt_opts;

	if (sys_adjtimex(&t) >= 0)
		timePropertiesDS.currentUtcOffset = t.tai;

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	/* This just allocates the stuff */
	pp_init_globals(ppg, &__pp_default_rt_opts);

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
