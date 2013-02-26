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

struct pp_ext_hooks pp_hooks = {
	.init = wr_init,
};

