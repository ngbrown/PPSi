/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

/*
 * This is the startup thing for hosted environments. It
 * defines main and then calls the main loop.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timex.h>

#include <ppsi/ppsi.h>
#include "ppsi-unix.h"

/* ppg and fields */
static struct pp_globals ppg_static;
static DSDefault defaultDS;
static DSCurrent currentDS;
static DSParent parentDS;
static DSTimeProperties timePropertiesDS;
static struct pp_servo servo;

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	struct timex t;
	int i;

	setbuf(stdout, NULL);

	pp_printf("PPSi. Commit %s, built on " __DATE__ "\n", PPSI_VERSION);

	ppg = &ppg_static;
	ppg->defaultDS = &defaultDS;
	ppg->currentDS = &currentDS;
	ppg->parentDS = &parentDS;
	ppg->timePropertiesDS = &timePropertiesDS;
	ppg->servo = &servo;
	ppg->rt_opts = &__pp_default_rt_opts;

	/* We are hosted, so we can allocate */
	ppg->max_links = PP_MAX_LINKS;
	ppg->arch_data = calloc(1, sizeof(struct unix_arch_data));
	ppg->pp_instances = calloc(ppg->max_links, sizeof(struct pp_instance));

	if ((!ppg->arch_data) || (!ppg->pp_instances))
		exit(__LINE__);

	/* Set offset here, so config parsing can override it */
	if (adjtimex(&t) >= 0)
		timePropertiesDS.currentUtcOffset = t.tai;

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	/* If no item has been parsed, provide a default file or string */
	if (ppg->cfg.cfg_items == 0)
		pp_config_file(ppg, 0, PP_DEFAULT_CONFIGFILE);
	if (ppg->cfg.cfg_items == 0)
		pp_config_string(ppg, strdup("link 0; iface eth0; proto udp"));

	for (i = 0; i < ppg->nlinks; i++) {

		ppi = INST(ppg, i);
		NP(ppi)->ch[PP_NP_EVT].fd = -1;
		NP(ppi)->ch[PP_NP_GEN].fd = -1;

		ppi->glbs = ppg;
		ppi->iface_name = ppi->cfg.iface_name;
		/* this old-fashioned "ethernet_mode" is a single bit */
		ppi->ethernet_mode = (ppi->cfg.proto == PPSI_PROTO_RAW);
		if (ppi->cfg.role == PPSI_ROLE_MASTER) {
			ppi->master_only = 1;
			ppi->slave_only = 0;
		}
		else if (ppi->cfg.role == PPSI_ROLE_SLAVE) {
			ppi->master_only = 0;
			ppi->slave_only = 1;
		}

		/* FIXME set ppi ext enable as defined in its pp_link */

		ppi->portDS = calloc(1, sizeof(*ppi->portDS));

		/* The following default names depend on TIME= at build time */
		ppi->n_ops = &DEFAULT_NET_OPS;
		ppi->t_ops = &DEFAULT_TIME_OPS;

		if (!ppi->portDS)
			exit(__LINE__);
	}

	pp_init_globals(ppg, &__pp_default_rt_opts);

	unix_main_loop(ppg);
	return 0; /* never reached */
}
