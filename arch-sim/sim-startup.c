/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Pietro Fezzardi (pietrofezzardi@gmail.com)
 *
 * Released to the public domain
 */

#include <stdio.h>

#include <ppsi/ppsi.h>
#include "ppsi-sim.h"

static struct pp_runtime_opts sim_master_rt_opts = {
	.clock_quality = {
			.clockClass = PP_CLASS_WR_GM_LOCKED,
			.clockAccuracy = PP_DEFAULT_CLOCK_ACCURACY,
			.offsetScaledLogVariance = PP_DEFAULT_CLOCK_VARIANCE,
	},
	.inbound_latency =	{0, PP_DEFAULT_INBOUND_LATENCY},
	.outbound_latency =	{0, PP_DEFAULT_OUTBOUND_LATENCY},
	.max_rst =		PP_DEFAULT_MAX_RESET,
	.max_dly =		PP_DEFAULT_MAX_DELAY,
	.flags =		PP_DEFAULT_FLAGS,
	.ap =			PP_DEFAULT_AP,
	.ai =			PP_DEFAULT_AI,
	.s =			PP_DEFAULT_DELAY_S,
	.announce_intvl =	PP_DEFAULT_ANNOUNCE_INTERVAL,
	.sync_intvl =		PP_DEFAULT_SYNC_INTERVAL,
	.prio1 =		PP_DEFAULT_PRIORITY1,
	.prio2 =		PP_DEFAULT_PRIORITY2,
	.domain_number =	PP_DEFAULT_DOMAIN_NUMBER,
	.ttl =			PP_DEFAULT_TTL,
};

/*
 * In arch-sim we use two pp_instaces in the same pp_globals to represent
 * two different machines. This means *completely differnt* machines, with
 * their own Data Sets. Given we can't put more all the different Data Sets
 * in the same ppg, we stored them in the ppi->arch_data of every istance.
 * This function is used to set the inner Data Sets pointer of the ppg to
 * point to the Data Sets related to the pp_instange passed as argument
 */
int sim_set_global_DS(struct pp_instance *ppi)
{
	struct sim_ppi_arch_data *data = SIM_PPI_ARCH(ppi);

	ppi->glbs->defaultDS = data->defaultDS;
	ppi->glbs->currentDS = data->currentDS;
	ppi->glbs->parentDS = data->parentDS;
	ppi->glbs->timePropertiesDS = data->timePropertiesDS;
	ppi->glbs->servo = data->servo;
	ppi->glbs->rt_opts = data->rt_opts;

	return 0;
}

static int sim_ppi_init(struct pp_instance *ppi, int which_ppi)
{
	struct sim_ppi_arch_data *data;
	ppi->proto = PP_DEFAULT_PROTO;
	ppi->__tx_buffer = malloc(PP_MAX_FRAME_LENGTH);
	ppi->__rx_buffer = malloc(PP_MAX_FRAME_LENGTH);
	ppi->arch_data = calloc(1, sizeof(struct sim_ppi_arch_data));
	ppi->portDS = calloc(1, sizeof(*ppi->portDS));
	if ((!ppi->arch_data) || (!ppi->portDS))
		return -1;
	data = SIM_PPI_ARCH(ppi);
	data->defaultDS = calloc(1, sizeof(*data->defaultDS));
	data->currentDS = calloc(1, sizeof(*data->currentDS));
	data->parentDS = calloc(1, sizeof(*data->parentDS));
	data->timePropertiesDS = calloc(1,
				sizeof(*data->timePropertiesDS));
	data->servo = calloc(1, sizeof(*data->servo));
	if ((!data->defaultDS) ||
			(!data->currentDS) ||
			(!data->parentDS) ||
			(!data->timePropertiesDS) ||
			(!data->servo))
		return -1;
	if (which_ppi == SIM_MASTER)
		data->rt_opts = &sim_master_rt_opts;
	else
		data->rt_opts = &__pp_default_rt_opts;
	data->other_ppi = INST(ppi->glbs, -(which_ppi - 1));
	return 0;
}

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	int i;

	setbuf(stdout, NULL);
	pp_printf("PPSi. Commit %s, built on " __DATE__ "\n", PPSI_VERSION);

	ppg = calloc(1, sizeof(struct pp_globals));
	ppg->max_links = 2; // master and slave, nothing else
	ppg->arch_data = calloc(1, sizeof(struct sim_ppg_arch_data));
	ppg->pp_instances = calloc(ppg->max_links, sizeof(struct pp_instance));

	if ((!ppg->arch_data) || (!ppg->pp_instances))
		return -1;

	/* Alloc data stuctures inside the pp_instances */
	for (i = 0; i < ppg->max_links; i++) {
		ppi = INST(ppg, i);
		ppi->glbs = ppg; // must be done before using sim_set_global_DS
		if (sim_ppi_init(ppi, i))
			return -1;
	}

	/*
	 * Configure the master with standard configuration, only from default
	 * string. The master is not configurable, but there's no need to do
	 * it cause we are ok with a standard one. We just want to see the
	 * behaviour of the slave.
	 * NOTE: the master instance is initialized before parsing the command
	 * line, so the diagnostics cannot be enabled here. We cannot put the
	 * master config later because the initial time for the master is needed
	 * to set the initial offset for the slave
	 */
	sim_set_global_DS(pp_sim_get_master(ppg));
	pp_config_string(ppg, strdup("port SIM_MASTER; iface MASTER;"
					"proto udp; role master;"
					"sim_iter_max 10000;"
					"sim_init_master_time .9;"));

	/* parse commandline for configuration options */
	sim_set_global_DS(pp_sim_get_slave(ppg));
	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;
	/* If no item has been parsed, provide default file or string */
	if (ppg->cfg.cfg_items == 0)
		pp_config_file(ppg, 0, PP_DEFAULT_CONFIGFILE);
	if (ppg->cfg.cfg_items == 0)
		pp_config_string(ppg, strdup("port SIM_SLAVE; iface SLAVE;"
						"proto udp; role slave;"));

	for (i = 0; i < ppg->nlinks; i++) {
		ppi = INST(ppg, i);
		sim_set_global_DS(ppi);
		ppi->iface_name = ppi->cfg.iface_name;
		ppi->port_name = ppi->cfg.port_name;
		if (ppi->proto == PPSI_PROTO_RAW)
			pp_printf("Warning: simulator doesn't support raw "
					"ethernet. Using UDP\n");
		NP(ppi)->ch[PP_NP_GEN].fd = -1;
		NP(ppi)->ch[PP_NP_EVT].fd = -1;
		ppi->t_ops = &DEFAULT_TIME_OPS;
		ppi->n_ops = &DEFAULT_NET_OPS;
		if (pp_sim_is_master(ppi))
			pp_init_globals(ppg, &sim_master_rt_opts);
		else
			pp_init_globals(ppg, &__pp_default_rt_opts);
	}

	sim_main_loop(ppg);
	return 0;
}
