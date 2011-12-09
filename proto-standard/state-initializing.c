/*
 * FIXME: header
 */
#include <pproto/pproto.h>
#include <dep/dep.h>

/*
 * Initializes network and other stuff
 */

int pp_initializing(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct pp_runtime_opts *rt_opts = ppi->rt_opts;

	pp_net_shutdown(ppi);
	
	if (pp_net_init(ppi) < 0)
		goto failure;

	/* Initialize default data set */
	ppi->defaultDS->twoStepFlag = PP_TWO_STEP_FLAG;
	/* TODO initialize clockIdentity with MAC address */
	ppi->defaultDS->clockIdentity[3] = 0xff;
	ppi->defaultDS->clockIdentity[4] = 0xfe;
	ppi->defaultDS->numberPorts = 1;
	pp_memcpy(&ppi->defaultDS->clockQuality, &rt_opts->clock_quality,
		sizeof(ClockQuality));
	ppi->defaultDS->priority1 = rt_opts->prio1;
	ppi->defaultDS->priority2 = rt_opts->prio2;
	ppi->defaultDS->domainNumber = rt_opts->domain_number;
	ppi->defaultDS->slaveOnly = rt_opts->slave_only;
	if (rt_opts->slave_only)
		ppi->defaultDS->clockQuality.clockClass = 255;

	/* Initialize port data set */
	pp_memcpy(ppi->portDS->portIdentity.clockIdentity,
		ppi->defaultDS->clockIdentity, PP_CLOCK_IDENTITY_LENGTH);
	ppi->portDS->portIdentity.portNumber = 1;
	ppi->portDS->logMinDelayReqInterval = PP_DEFAULT_DELAYREQ_INTERVAL;
	ppi->portDS->peerMeanPathDelay.seconds = 0;
	ppi->portDS->peerMeanPathDelay.nanoseconds = 0;
	ppi->portDS->logAnnounceInterval = rt_opts->announce_intvl;
	ppi->portDS->announceReceiptTimeout =
		PP_DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
	ppi->portDS->logSyncInterval = rt_opts->sync_intvl;
	ppi->portDS->delayMechanism = PP_DEFAULT_DELAY_MECHANISM;
	ppi->portDS->logMinPdelayReqInterval = PP_DEFAULT_PDELAYREQ_INTERVAL;
	ppi->portDS->versionNumber = PP_VERSION_PTP;

	if (pp_timer_init(ppi))
		goto failure;

	/* TODO Check the following code coming from ptpd.
	 *
	 * Init all remaining stuff:
	 * initClock(rtOpts, ptpClock);
	 *
	 * Set myself as master (in case will not receive any announce):
	 * m1(ptpClock);
	 *
	 * Prepare a message:
	 * msgPackHeader(ptpClock->msgObuf,ptpClock);
	 */

	ppi->next_state = PPS_LISTENING;
	return 0;

failure:
	pp_printf("Failed to initialize network\n");
	ppi->next_state = PPS_FAULTY;
	return 0;
}
