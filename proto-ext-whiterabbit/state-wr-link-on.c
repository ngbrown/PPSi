/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

/*
 * This is the last WR state: ack the other party and go master or slave.
 * There is no timeout nor a check for is_new_state: we just do things once
 */
int wr_link_on(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);
	int e = 0;

	wrp->wrModeOn = TRUE;
	wrp->ops->enable_ptracker(ppi);

	if (wrp->wrMode == WR_MASTER)
		e = msg_issue_wrsig(ppi, WR_MODE_ON);

	wrp->parentWrModeOn = TRUE;

	if (e != 0)
		return -1;

	if (wrp->wrMode == WR_SLAVE)
		ppi->next_state = PPS_SLAVE;
	else
		ppi->next_state = PPS_MASTER;
	return 0;
}
