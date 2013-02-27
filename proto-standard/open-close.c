/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

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
	.ethernet_mode =	PP_DEFAULT_ETHERNET_MODE,
	.e2e_mode =		PP_DEFAULT_E2E_MODE,
	.gptp_mode =		PP_DEFAULT_GPTP_MODE,
	.ofst_first_updated =	0, /* FIXME: is it a option or a state var? */
	.max_foreign_records =	PP_DEFAULT_MAX_FOREIGN_RECORDS,
	.cur_utc_ofst =		PP_DEFAULT_UTC_OFFSET,
	.announce_intvl =	PP_DEFAULT_ANNOUNCE_INTERVAL,
	.sync_intvl =		PP_DEFAULT_SYNC_INTERVAL,
	.prio1 =		PP_DEFAULT_PRIORITY1,
	.prio2 =		PP_DEFAULT_PRIORITY2,
	.domain_number =	PP_DEFAULT_DOMAIN_NUMBER,
	.ttl =			PP_DEFAULT_TTL,
};

int pp_open_instance(struct pp_instance *ppi, struct pp_runtime_opts *rt_opts)
{
	if (rt_opts)
		ppi->rt_opts = rt_opts;
	else
		ppi->rt_opts = &default_rt_opts;

	ppi->state = PPS_INITIALIZING;

	if (pp_hooks.open)
		return pp_hooks.open(ppi, ppi->rt_opts);
	return 0;
}

int pp_close_instance(struct pp_instance *ppi)
{
	if (pp_hooks.close)
		return pp_hooks.close(ppi);
	return 0;
}
