/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */
#include <ppsi/ppsi.h>
#include "wr-api.h"

/* We are entering WR handshake, as either master or slave */
void wr_handshake_init(struct pp_instance *ppi, int mode_or_retry)
{
        struct wr_dsport *wrp = WR_DSPOR(ppi);

	switch(mode_or_retry) {

	case PPS_MASTER:
		wrp->wrMode = WR_MASTER;
		ppi->next_state = WRS_M_LOCK;
		break;

	case PPS_SLAVE:
		wrp->wrMode = WR_SLAVE;
		ppi->next_state = WRS_PRESENT;
		break;

	default: /* retry: only called from below in this file */
		if (wrp->wrMode == WR_MASTER)
			ppi->next_state = WRS_M_LOCK;
		else
			ppi->next_state = WRS_PRESENT;
		break;
	}
}

/* The handshake failed: go master or slave in normal PTP mode */
void wr_handshake_fail(struct pp_instance *ppi)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);

	pp_diag(ppi, ext, 1, "Handshake failure: now non-wr %s\n",
		wrp->wrMode == WR_MASTER ? "master" : "slave");
	if (wrp->wrMode == WR_MASTER)
		ppi->next_state = PPS_MASTER;
	else
		ppi->next_state = PPS_SLAVE;
	wrp->wrMode = NON_WR;
}


/* One of the steps failed: either retry or fail */
int wr_handshake_retry(struct pp_instance *ppi)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);

	if (wrp->wrStateRetry > 0) {
		wrp->wrStateRetry--;
		pp_diag(ppi, ext, 1, "Retry on timeout\n");
		return 1; /* yes, retry */
	}
	wr_handshake_fail(ppi);
	return 0; /* don't retry, we are over already */
}
