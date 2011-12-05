/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <pproto/pproto.h>

/*
 * This file deals with opening and closing an instance. The channel
 * must already have been created. In practices, this initializes the
 * state machine to the first state.
 *
 * A protocol extension can override none or both of these functions.
 */

struct pp_runtime_opts default_rt_opts = {
	/* TODO */
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
