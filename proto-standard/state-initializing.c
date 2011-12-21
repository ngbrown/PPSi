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
	DSDEF(ppi)->twoStepFlag = PP_TWO_STEP_FLAG;
	/* TODO initialize clockIdentity with MAC address */
	DSDEF(ppi)->clockIdentity[3] = 0xff;
	DSDEF(ppi)->clockIdentity[4] = 0xfe;
	DSDEF(ppi)->numberPorts = 1;
	pp_memcpy(&DSDEF(ppi)->clockQuality, &rt_opts->clock_quality,
		sizeof(ClockQuality));
	DSDEF(ppi)->priority1 = rt_opts->prio1;
	DSDEF(ppi)->priority2 = rt_opts->prio2;
	DSDEF(ppi)->domainNumber = rt_opts->domain_number;
	DSDEF(ppi)->slaveOnly = rt_opts->slave_only;
	if (rt_opts->slave_only)
		ppi->defaultDS->clockQuality.clockClass = 255;

	/* Initialize port data set */
	pp_memcpy(ppi->portDS->portIdentity.clockIdentity,
		ppi->defaultDS->clockIdentity, PP_CLOCK_IDENTITY_LENGTH);
	DSPOR(ppi)->portIdentity.portNumber = 1;
	DSPOR(ppi)->logMinDelayReqInterval = PP_DEFAULT_DELAYREQ_INTERVAL;
	DSPOR(ppi)->peerMeanPathDelay.seconds = 0;
	DSPOR(ppi)->peerMeanPathDelay.nanoseconds = 0;
	DSPOR(ppi)->logAnnounceInterval = rt_opts->announce_intvl;
	DSPOR(ppi)->announceReceiptTimeout =
		PP_DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
	DSPOR(ppi)->logSyncInterval = rt_opts->sync_intvl;
	DSPOR(ppi)->delayMechanism = PP_DEFAULT_DELAY_MECHANISM;
	DSPOR(ppi)->logMinPdelayReqInterval = PP_DEFAULT_PDELAYREQ_INTERVAL;
	DSPOR(ppi)->versionNumber = PP_VERSION_PTP;

	if (pp_timer_init(ppi))
		goto failure;

	/* TODO Check the following code coming from ptpd.
	 *
	 * Init all remaining stuff:
	 * initClock(rtOpts, ptpClock);
	 */

	m1(ppi);

	msg_pack_header(ppi->buf_out,ppi);

	ppi->next_state = PPS_LISTENING;
	return 0;

failure:
	pp_printf("Failed to initialize network\n");
	ppi->next_state = PPS_FAULTY;
	return 0;
}
