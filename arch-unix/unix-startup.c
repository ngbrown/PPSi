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


#include <ppsi/ppsi.h>
#include "ppsi-unix.h"

CONST_VERBOSITY int pp_diag_verbosity = 0;

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	int i;

	setbuf(stdout, NULL);

	/* We are hosted, so we can allocate */
	ppg = calloc(1, sizeof(*ppg));
	if (!ppg)
		exit(__LINE__);

	ppg->max_links = PP_MAX_LINKS;
	ppg->links = calloc(ppg->max_links, sizeof(struct pp_link));
	ppg->defaultDS = calloc(1, sizeof(*ppg->defaultDS));
	ppg->currentDS = calloc(1, sizeof(*ppg->currentDS));
	ppg->parentDS = calloc(1, sizeof(*ppg->parentDS));
	ppg->timePropertiesDS = calloc(1, sizeof(*ppg->timePropertiesDS));
	ppg->arch_data = calloc(1, sizeof(struct unix_arch_data));
	ppg->pp_instances = calloc(ppg->max_links, sizeof(struct pp_instance));
	ppg->servo = calloc(1, sizeof(*ppg->servo));
	ppg->rt_opts = &default_rt_opts;

	if ((!ppg->defaultDS) || (!ppg->currentDS) || (!ppg->parentDS)
		|| (!ppg->timePropertiesDS) || (!ppg->arch_data)
		|| (!ppg->pp_instances) || (!ppg->servo))
		exit(__LINE__);

	pp_config_file(ppg, &argc, argv, NULL,
		       "link 0\niface eth0\n" /* mandatory trailing \n */);

	for (i = 0; i < ppg->nlinks; i++) {

		struct pp_link *lnk = &ppg->links[i];

		ppi = &ppg->pp_instances[i];
		ppi->glbs = ppg;
		ppi->iface_name = lnk->iface_name;
		ppi->ethernet_mode = (lnk->proto == 0) ? 1 : 0;
		if (lnk->role == 1) {
			ppi->master_only = 1;
			ppi->slave_only = 0;
		}
		else if (lnk->role == 2) {
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
