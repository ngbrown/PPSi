/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

#include <ppsi/ppsi.h>

/*
 * This file deals with opening and closing an instance. The channel
 * must already have been created. In practice, this initializes the
 * state machine to the first state.
 */

struct pp_runtime_opts default_rt_opts = {
	.clock_quality = {
			.clockClass = PP_DEFAULT_CLOCK_CLASS,
			.clockAccuracy = PP_DEFAULT_CLOCK_ACCURACY,
			.offsetScaledLogVariance = PP_DEFAULT_CLOCK_VARIANCE,
	},
	.inbound_latency =	{0, PP_DEFAULT_INBOUND_LATENCY},
	.outbound_latency =	{0, PP_DEFAULT_OUTBOUND_LATENCY},
	.max_rst =		PP_DEFAULT_MAX_RESET,
	.max_dly =		PP_DEFAULT_MAX_DELAY,
	.no_adjust =		PP_DEFAULT_NO_ADJUST,
	.no_rst_clk =		PP_DEFAULT_NO_RESET_CLOCK,
	.ap =			PP_DEFAULT_AP,
	.ai =			PP_DEFAULT_AI,
	.s =			PP_DEFAULT_DELAY_S,
	.ofst_first_updated =	0, /* FIXME: is it a option or a state var? */
	.announce_intvl =	PP_DEFAULT_ANNOUNCE_INTERVAL,
	.sync_intvl =		PP_DEFAULT_SYNC_INTERVAL,
	.prio1 =		PP_DEFAULT_PRIORITY1,
	.prio2 =		PP_DEFAULT_PRIORITY2,
	.domain_number =	PP_DEFAULT_DOMAIN_NUMBER,
	.ttl =			PP_DEFAULT_TTL,
};

int pp_open_globals(struct pp_globals *ppg, struct pp_runtime_opts *_rt_opts)
{
	/*
	 * Initialize default data set
	 */
	int i;
	struct DSDefault *def = ppg->defaultDS;
	def->twoStepFlag = TRUE;
	def->numberPorts = ppg->nlinks;
	struct pp_runtime_opts *rt_opts;

	if (_rt_opts)
		rt_opts = _rt_opts;
	else
		rt_opts = &default_rt_opts;

	ppg->rt_opts = rt_opts;

	memcpy(&def->clockQuality, &rt_opts->clock_quality,
		   sizeof(ClockQuality));

	if (ppg->nlinks == 1) {
		def->slaveOnly = ppg->pp_instances[0].slave_only;
	}
	else
		def->slaveOnly = 1; /* the for cycle below will set it to 0 if not
							 * ports are not all slave_only */

	def->priority1 = rt_opts->prio1;
	def->priority2 = rt_opts->prio2;
	def->domainNumber = rt_opts->domain_number;

	for (i = 0; i < ppg->nlinks; i++) {

		if (def->slaveOnly && !ppg->pp_instances[i].slave_only)
			def->slaveOnly = 0;

		ppg->pp_instances[i].state = PPS_INITIALIZING;
		ppg->pp_instances[i].port_idx = i;
	}

	if (def->slaveOnly)
		def->clockQuality.clockClass = 255;

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
