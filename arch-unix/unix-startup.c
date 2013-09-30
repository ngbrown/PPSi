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

CONST_VERBOSITY int pp_diag_verbosity = 0;

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

	ppg = &ppg_static;
	ppg->defaultDS = &defaultDS;
	ppg->currentDS = &currentDS;
	ppg->parentDS = &parentDS;
	ppg->timePropertiesDS = &timePropertiesDS;
	ppg->servo = &servo;
	ppg->rt_opts = &default_rt_opts;

	/* We are hosted, so we can allocate */
	ppg->max_links = PP_MAX_LINKS;
	ppg->arch_data = calloc(1, sizeof(struct unix_arch_data));
	ppg->pp_instances = calloc(ppg->max_links, sizeof(struct pp_instance));

	if ((!ppg->arch_data) || (!ppg->pp_instances))
		exit(__LINE__);

	/* Set offset here, so config parsing can override it */
	if (adjtimex(&t) >= 0)
		timePropertiesDS.currentUtcOffset = t.tai;

	pp_config_file(ppg, &argc, argv, NULL, "link 0\niface eth0\n"
		       "proto udp\n" /* mandatory  trailing \n */);

	for (i = 0; i < ppg->nlinks; i++) {

		ppi = &ppg->pp_instances[i];
		NP(ppi)->ch[PP_NP_EVT].fd = -1;
		NP(ppi)->ch[PP_NP_GEN].fd = -1;

		ppi->glbs = ppg;
		ppi->iface_name = ppi->cfg.iface_name;
		ppi->ethernet_mode = (ppi->cfg.proto == 0) ? 1 : 0;
		if (ppi->cfg.role == 1) {
			ppi->master_only = 1;
			ppi->slave_only = 0;
		}
		else if (ppi->cfg.role == 2) {
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

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	pp_open_globals(ppg);

	unix_main_loop(ppg);
	return 0; /* never reached */
}
