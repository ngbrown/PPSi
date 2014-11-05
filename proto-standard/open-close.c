/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

/*
 * This is a global structure, because commandline and other config places
 * need to change some values in there.
 */
struct pp_runtime_opts __pp_default_rt_opts = {
	.clock_quality = {
			.clockClass = PP_CLASS_DEFAULT,
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
 * This file deals with opening and closing an instance. The channel
 * must already have been created. In practice, this initializes the
 * state machine to the first state.
 */

int pp_init_globals(struct pp_globals *ppg, struct pp_runtime_opts *pp_rt_opts)
{
	/*
	 * Initialize default data set
	 */
	int i;
	struct DSDefault *def = ppg->defaultDS;
	def->twoStepFlag = TRUE;

	/* if ppg->nlinks == 0, let's assume that the 'pp_links style'
	 * configuration was not used, so we have 1 port */
	def->numberPorts = ppg->nlinks > 0 ? ppg->nlinks : 1;
	struct pp_runtime_opts *rt_opts;

	if (!ppg->rt_opts)
		ppg->rt_opts = pp_rt_opts;

	rt_opts = ppg->rt_opts;

	memcpy(&def->clockQuality, &rt_opts->clock_quality,
		   sizeof(ClockQuality));

	if (def->numberPorts == 1)
		def->slaveOnly = (INST(ppg, 0)->role == PPSI_ROLE_SLAVE);
	else
		def->slaveOnly = 1; /* the for cycle below will set it to 0 if not
							 * ports are not all slave_only */

	def->priority1 = rt_opts->prio1;
	def->priority2 = rt_opts->prio2;
	def->domainNumber = rt_opts->domain_number;

	for (i = 0; i < def->numberPorts; i++) {
		struct pp_instance *ppi = INST(ppg, i);

		if (def->slaveOnly && ppi->role != PPSI_ROLE_SLAVE)
			def->slaveOnly = 0;

		ppi->state = PPS_INITIALIZING;
		ppi->port_idx = i;
		ppi->frgn_rec_best = -1;
	}

	if (def->slaveOnly) {
		pp_printf("Slave Only, clock class set to 255\n");
		def->clockQuality.clockClass = PP_CLASS_SLAVE_ONLY;
	}

	if (pp_hooks.open)
		return pp_hooks.open(ppg, rt_opts);
	return 0;
}

int pp_close_globals(struct pp_globals *ppg)
{
	if (pp_hooks.close)
		return pp_hooks.close(ppg);
	return 0;
}
