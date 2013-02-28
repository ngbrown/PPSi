/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

/*
 * Initializes network and other stuff
 */

int pp_initializing(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	unsigned char *id, *mac;
	struct DSDefault *def = DSDEF(ppi);
	struct DSPort *port = DSPOR(ppi);
	struct pp_runtime_opts *opt = OPTS(ppi);
	int ret = 0;

	if (NP(ppi)->inited)
		pp_net_ops.exit(ppi);

	if (pp_net_ops.init(ppi) < 0)
		goto failure;

	NP(ppi)->inited = 1;

	port->portState = PPS_INITIALIZING;

	/* Initialize default data set */
	def->twoStepFlag = PP_TWO_STEP_FLAG;
	/* Clock identity comes from mac address with 0xff:0xfe intermixed */
	id = def->clockIdentity;
	mac = NP(ppi)->ch[PP_NP_GEN].addr;
	id[0] = mac[0];
	id[1] = mac[1];
	id[2] = mac[2];
	id[3] = 0xff;
	id[4] = 0xfe;
	id[5] = mac[3];
	id[6] = mac[4];
	id[7] = mac[5];

	def->numberPorts = 1;
	memcpy(&def->clockQuality, &opt->clock_quality,
		sizeof(ClockQuality));
	def->priority1 = opt->prio1;
	def->priority2 = opt->prio2;
	def->domainNumber = opt->domain_number;
	def->slaveOnly = opt->slave_only;
	if (opt->slave_only)
		ppi->defaultDS->clockQuality.clockClass = 255;

	/* Initialize port data set */
	memcpy(ppi->portDS->portIdentity.clockIdentity,
		ppi->defaultDS->clockIdentity, PP_CLOCK_IDENTITY_LENGTH);
	port->portIdentity.portNumber = 1;
	port->logMinDelayReqInterval = PP_DEFAULT_DELAYREQ_INTERVAL;
	clear_TimeInternal(&port->peerMeanPathDelay);
	port->logAnnounceInterval = opt->announce_intvl;
	port->announceReceiptTimeout = PP_DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
	port->logSyncInterval = opt->sync_intvl;
	port->delayMechanism = PP_DEFAULT_DELAY_MECHANISM;
	port->logMinPdelayReqInterval = PP_DEFAULT_PDELAYREQ_INTERVAL;
	port->versionNumber = PP_VERSION_PTP;

	if (pp_hooks.init)
		ret = pp_hooks.init(ppi, pkt, plen);
	if (ret) {
		PP_PRINTF("%s: can't init extension\n", __func__);
		goto failure;
	}

	ret = pp_timer_init(ppi);
	if (ret) {
		PP_PRINTF("%s: can't init timers\n", __func__);
		goto failure;
	}
	pp_init_clock(ppi);

	m1(ppi);

	msg_pack_header(ppi, ppi->buf_out);

	if (!opt->master_only)
		ppi->next_state = PPS_LISTENING;
	else
		ppi->next_state = PPS_MASTER;
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;

failure:
	ppi->next_state = PPS_FAULTY;
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return ret;
}
