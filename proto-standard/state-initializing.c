/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

/*
 * Initializes network and other stuff
 */

int pp_initializing(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	unsigned char *id, *mac;
	struct DSPort *port = DSPOR(ppi);
	struct pp_runtime_opts *opt = OPTS(ppi);
	int ret = 0;

	if (ppi->n_ops->init(ppi) < 0) /* it must handle being called twice */
		goto failure;

	/* Clock identity comes from mac address with 0xff:0xfe intermixed */
	id = (unsigned char *)&DSDEF(ppi)->clockIdentity;
	mac = NP(ppi)->ch[PP_NP_GEN].addr;
	id[0] = mac[0];
	id[1] = mac[1];
	id[2] = mac[2];
	id[3] = 0xff;
	id[4] = 0xfe;
	id[5] = mac[3];
	id[6] = mac[4];
	id[7] = mac[5];

	/*
	 * Initialize port data set
	 */
	memcpy(&port->portIdentity.clockIdentity,
		&DSDEF(ppi)->clockIdentity, PP_CLOCK_IDENTITY_LENGTH);
	/* 1-based port number =  index of this ppi in the global array */
	port->portIdentity.portNumber = 1 + ppi - ppi->glbs->pp_instances;
	port->logMinDelayReqInterval = PP_DEFAULT_DELAYREQ_INTERVAL;
	port->logAnnounceInterval = opt->announce_intvl;
	port->announceReceiptTimeout = PP_DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
	port->logSyncInterval = opt->sync_intvl;
	port->versionNumber = PP_VERSION_PTP;

	if (pp_hooks.init)
		ret = pp_hooks.init(ppi, pkt, plen);
	if (ret) {
		pp_diag(ppi, ext, 1, "%s: can't init extension\n", __func__);
		goto failure;
	}

	if (ret) {
		pp_diag(ppi, time, 1, "%s: can't init timers\n", __func__);
		goto failure;
	}

	pp_diag(ppi, bmc, 1, "clock class = %d\n",
			DSDEF(ppi)->clockQuality.clockClass);
	pp_diag(ppi, bmc, 1, "clock accuracy = %d\n",
			DSDEF(ppi)->clockQuality.clockAccuracy);

	m1(ppi);

	msg_pack_header(ppi, ppi->tx_ptp); /* This is used for all tx */

	if (ppi->role != PPSI_ROLE_MASTER)
		ppi->next_state = PPS_LISTENING;
	else
		ppi->next_state = PPS_MASTER;
	return 0;

failure:
	ppi->next_delay = 1000; /* wait 1s before retrying */
	return 0;
}
