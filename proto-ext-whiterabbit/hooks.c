#include <ppsi/ppsi.h>
#include "wr-api.h"

/* ext-whiterabbit must offer its own hooks */

static int wr_init(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	WR_DSPOR(ppi)->wrStateTimeout = WR_DEFAULT_STATE_TIMEOUT_MS;
	WR_DSPOR(ppi)->wrStateRetry = WR_DEFAULT_STATE_REPEAT;
	WR_DSPOR(ppi)->calPeriod = WR_DEFAULT_CAL_PERIOD;
	WR_DSPOR(ppi)->wrModeOn = 0;
	WR_DSPOR(ppi)->parentWrConfig = 0;
	WR_DSPOR(ppi)->calibrated = !WR_DEFAULT_PHY_CALIBRATION_REQUIRED;
	return 0;
}

struct pp_ext_hooks pp_hooks = {
	.init = wr_init,
};

