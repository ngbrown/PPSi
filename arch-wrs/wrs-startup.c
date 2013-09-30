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

#include <minipc.h>
#include <hal_exports.h>

#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>
#include "../proto-ext-whiterabbit/wr-api.h"

CONST_VERBOSITY int pp_diag_verbosity = 0;

struct minipc_ch *hal_ch;
struct minipc_ch *ppsi_ch;

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	int i;

	setbuf(stdout, NULL);

	hal_ch = minipc_client_create(WRSW_HAL_SERVER_ADDR,
				      MINIPC_FLAG_VERBOSE);
	if (!hal_ch) { /* FIXME should we retry with minipc_client_create? */
		pp_printf("Fatal: could not connect to HAL");
		exit(__LINE__);
	}

	ppsi_ch = minipc_server_create("ptpd", 0);
	if (!ppsi_ch) { /* FIXME should we retry with minipc_server_create? */
		pp_printf("Fatal: could not create minipc server");
		exit(__LINE__);
	}
	wrs_init_ipcserver(ppsi_ch);

	/* We are hosted, so we can allocate */
	ppg = calloc(1, sizeof(*ppg));
	if (!ppg)
		exit(__LINE__);

	ppg->max_links = PP_MAX_LINKS;
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

	pp_config_file(ppg, &argc, argv, "/wr/etc/ppsi.conf",
		       "link 0\niface wr0\n" /* mandatory trailing \n */);

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

		ppi->portDS = calloc(1, sizeof(*ppi->portDS));
		if (!ppi->portDS)
			exit(__LINE__);

		ppi->portDS->ext_dsport = calloc(1, sizeof(struct wr_dsport));
		if (!ppi->portDS->ext_dsport)
			exit(__LINE__);

		/* The following default names depend on TIME= at build time */
		ppi->n_ops = &DEFAULT_NET_OPS;
		ppi->t_ops = &DEFAULT_TIME_OPS;

	}

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	pp_open_globals(ppg);

	wrs_main_loop(ppg);
	return 0; /* never reached */
}
