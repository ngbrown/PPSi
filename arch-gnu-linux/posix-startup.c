/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/*
 * This is the startup thing for hosted environments. It
 * defines main and then calls the main loop.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ppsi/ppsi.h>
#include "posix.h"

CONST_VERBOSITY int pp_diag_verbosity = 0;

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	char *ifname, *nports;
	char tmp[16];
	int i = 0;

	setbuf(stdout, NULL);

	/* We are hosted, so we can allocate */
	ppg = calloc(1, sizeof(*ppg));
	if (!ppg)
		exit(__LINE__);

	nports = getenv("PPROTO_IF_NUM");
	if (nports)
		ppg->nports = atoi(nports);
	else
		ppg->nports = 1;

	ppg->pp_instances = calloc(ppg->nports, sizeof(struct pp_instance));

	if (!ppg->pp_instances)
		exit(__LINE__);

	for (; i < ppg->nports; i++) {

		sprintf(tmp, "PPROTO_IF_%02d", i);

		ifname = getenv(tmp);

		if (!ifname) {
			sprintf(tmp, "eth%d", i);
			ifname = tmp;
		}

		ppi = &ppg->pp_instances[i];
		ppi->glbs = ppg;

		/* FIXME check all of these calloc's, since some stuff will be
		 * part of pp_globals */
		ppi->defaultDS = calloc(1, sizeof(*ppi->defaultDS));
		ppi->currentDS = calloc(1, sizeof(*ppi->currentDS));
		ppi->parentDS = calloc(1, sizeof(*ppi->parentDS));
		ppi->portDS = calloc(1, sizeof(*ppi->portDS));
		ppi->timePropertiesDS = calloc(1, sizeof(*ppi->timePropertiesDS));
		ppi->servo = calloc(1, sizeof(*ppi->servo));
		ppi->arch_data = calloc(1, sizeof(struct posix_arch_data));

		ppi->n_ops = &posix_net_ops;
		ppi->t_ops = &posix_time_ops;

		if ((!ppi->defaultDS) || (!ppi->currentDS) || (!ppi->parentDS)
			|| (!ppi->portDS) || (!ppi->timePropertiesDS)
			|| (!GLBS(ppi)->frgn_master) || (!ppi->arch_data)
		)
			exit(__LINE__);

		pp_open_instance(ppi, NULL);

		/* FIXME temporary workaround to make the first interface work as in the past */
		if (i == 0)
			OPTS(ppi)->iface_name = strdup(ifname);
	}


	/* FIXME temporary workaround: pp_parse_cmdline will receive ppg */
	if (pp_parse_cmdline(&ppg->pp_instances[0], argc, argv) != 0)
		return -1;

	posix_main_loop(ppg);
	return 0; /* never reached */
}
