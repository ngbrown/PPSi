/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

int wr_s_lock(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);
	int enable = 0;

	if (ppi->is_new_state) {
                wrp->wrStateRetry = WR_STATE_RETRY;
		enable = 1;
	} else if (pp_timeout_z(ppi, PP_TO_EXT_0)) {
		wrp->ops->locking_disable(ppi);
		if (wr_handshake_retry(ppi))
			enable = 1;
		else
			return 0; /* non-wr already */
	}

	if (enable) {
		wrp->ops->locking_enable(ppi);
		pp_timeout_set(ppi, PP_TO_EXT_0, WR_S_LOCK_TIMEOUT_MS);
	}

	if (wrp->ops->locking_poll(ppi, 0) == WR_SPLL_READY) {
		ppi->next_state = WRS_LOCKED;
		wrp->ops->locking_disable(ppi);
	}

	ppi->next_delay = wrp->wrStateTimeout;
	return 0;
}
