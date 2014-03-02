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

#include <minipc.h>
#include <hal_exports.h>

#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>

static struct wr_operations wrs_wr_operations = {
	.locking_enable = wrs_locking_enable,
	.locking_poll = wrs_locking_poll,
	.locking_disable = wrs_locking_disable,
	.enable_ptracker = wrs_enable_ptracker,

	.adjust_in_progress = wrs_adjust_in_progress,
	.adjust_counters = wrs_adjust_counters,
	.adjust_phase = wrs_adjust_phase,

	.read_calib_data = wrs_read_calibration_data,
	.calib_disable = wrs_calibrating_disable,
	.calib_enable = wrs_calibrating_enable,
	.calib_poll = wrs_calibrating_poll,
	.calib_pattern_enable = wrs_calibration_pattern_enable,
	.calib_pattern_disable = wrs_calibration_pattern_disable,

	.enable_timing_output = wrs_enable_timing_output,
};

/* ppg and fields */
static struct pp_globals ppg_static;
static DSDefault defaultDS;
static DSCurrent currentDS;
static DSParent parentDS;
static DSTimeProperties timePropertiesDS;
static struct pp_servo servo;

struct minipc_ch *hal_ch;
struct minipc_ch *ppsi_ch;

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	struct wr_dsport *wrp;
	struct timex t;
	int i;

	setbuf(stdout, NULL);

	pp_printf("PPSi. Commit %s, built on " __DATE__ "\n",
		PPSI_VERSION);

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
	if (adjtimex(&t) >= 0) {
		int *p;
		/*
		 * Our WRS kernel has tai support, but our compiler does not.
		 * We are 32-bit only, and we know for sure that tai is
		 * exactly after stbcnt. It's a bad hack, but it works
		 */
		p = (int *)(&t.stbcnt) + 1;
		timePropertiesDS.currentUtcOffset = *p;
	}

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	/* If no item has been parsed, provide a default file or string */
	if (ppg->cfg_items == 0)
		pp_config_file(ppg, 0, "/wr/etc/ppsi.conf");
	if (ppg->cfg_items == 0)
		pp_config_file(ppg, 0, PP_DEFAULT_CONFIGFILE);
	if (ppg->cfg_items == 0) {
		/* Default configuration for WR switch is all ports */
		char s[128];
		int i;

		for (i = 0; i < 18; i++) {
			sprintf(s, "port wr%i; iface wr%i; proto raw;"
				"extension whiterabbit; role auto", i, i);
			pp_config_string(ppg, s);
		}
	}
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

		ppi->portDS = calloc(1, sizeof(*ppi->portDS));
		if (!ppi->portDS)
			exit(__LINE__);

		ppi->portDS->ext_dsport = calloc(1, sizeof(struct wr_dsport));
		if (!ppi->portDS->ext_dsport)
			exit(__LINE__);
		wrp = WR_DSPOR(ppi); /* just allocated above */
		wrp->ops = &wrs_wr_operations;

		/* The following default names depend on TIME= at build time */
		ppi->n_ops = &DEFAULT_NET_OPS;
		ppi->t_ops = &DEFAULT_TIME_OPS;

	}

	pp_init_globals(ppg, &__pp_default_rt_opts);

	wrs_main_loop(ppg);
	return 0; /* never reached */
}
