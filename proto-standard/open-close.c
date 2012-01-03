/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <pptp/pptp.h>

/*
 * This file deals with opening and closing an instance. The channel
 * must already have been created. In practices, this initializes the
 * state machine to the first state.
 *
 * A protocol extension can override none or both of these functions.
 */

struct pp_runtime_opts default_rt_opts = {
	.max_rst = FALSE,
	.max_dly = FALSE,
	.slave_only = FALSE,
	.no_adjust = TRUE,
	.display_stats = FALSE,
	.csv_stats = FALSE,
	.ethernet_mode = FALSE,
	.e2e_mode = FALSE,
	.ofst_first_updated = FALSE,
	.no_rst_clk = PP_DEFAULT_NO_RESET_CLOCK,
	.use_syslog = FALSE,
	.ap = PP_DEFAULT_AP,
	.ai = PP_DEFAULT_AI,
	.s = PP_DEFAULT_DELAY_S,
	.max_foreign_records = PP_DEFAULT_MAX_FOREIGN_RECORDS,
	.cur_utc_ofst = PP_DEFAULT_UTC_OFFSET,
	.announce_intvl = PP_DEFAULT_ANNOUNCE_INTERVAL,
	.sync_intvl = PP_DEFAULT_SYNC_INTERVAL,
	.prio1 = PP_DEFAULT_PRIORITY1,
	.prio2 = PP_DEFAULT_PRIORITY2,
	.domain_number = PP_DEFAULT_DOMAIN_NUMBER,
	.unicast_addr = 0,
	.iface_name = 0,
	.ttl = 1,
	.arch_opts = 0,
};


struct pp_runtime_opts *pp_get_default_rt_opts()
{
	return &default_rt_opts;
}

int pp_parse_cmdline(struct pp_instance *ppi, int argc, char **argv)
{
	/* TODO: Check how to parse cmdline. We can not rely on getopt()
	 * function. Which are the really useful parameters?
	 * Every parameter should be contained in ppi->rt_opts and set here
	 */
	return 0;
}

int pp_open_instance(struct pp_instance *ppi, struct pp_runtime_opts *rt_opts)
{
	if (rt_opts) {
		ppi->rt_opts = rt_opts;
	}
	else {
		ppi->rt_opts = &default_rt_opts;
	}
	ppi->state = PPS_INITIALIZING;

	return 0;
}

int pp_close_instance(struct pp_instance *ppi)
{
	/* Nothing to do by now */
	return 0;
}
