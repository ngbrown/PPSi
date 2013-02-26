#include <ppsi/ppsi.h>
#include "wr-api.h"

/* ext-whiterabbit must offer its own hooks */

static int wr_init(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wp = WR_DSPOR(ppi);

	wp->wrStateTimeout = WR_DEFAULT_STATE_TIMEOUT_MS;
	wp->wrStateRetry = WR_DEFAULT_STATE_REPEAT;
	wp->calPeriod = WR_DEFAULT_CAL_PERIOD;
	wp->wrModeOn = 0;
	wp->parentWrConfig = 0;
	wp->calibrated = !WR_DEFAULT_PHY_CALIBRATION_REQUIRED;
	return 0;
}

/* This currently only works with one interface (i.e. WR Node) */
static int wr_open(struct pp_instance *ppi, struct pp_runtime_opts *rt_opts)
{
	static struct wr_data_t wr_data; /* WR-specific global data */

	rt_opts->iface_name = "wr1";
	ppi->ext_data = &wr_data;
	return 0;
}


struct pp_ext_hooks pp_hooks = {
	.init = wr_init,
	.open = wr_open,
};

