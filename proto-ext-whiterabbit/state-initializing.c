/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "wr-api.h"

/*
 * Initializes network and other stuff
 */
static int pp_did_init = 0;

#define LOCK_TIMEOUT_GM (60000)

int pp_initializing(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	unsigned char *id, *mac;

	if (pp_did_init)
		pp_net_shutdown(ppi);

	if (pp_net_init(ppi) < 0)
		goto failure;
	pp_did_init = 1;
	DSPOR(ppi)->portState = PPS_INITIALIZING;

	/* Initialize default data set */
	DSDEF(ppi)->twoStepFlag = PP_TWO_STEP_FLAG;
	/* Clock identity comes from mac address with 0xff:0xfe intermixed */
	id = DSDEF(ppi)->clockIdentity;
	mac = NP(ppi)->ch[PP_NP_GEN].addr;
	id[0] = mac[0];
	id[1] = mac[1];
	id[2] = mac[2];
	id[3] = 0xff;
	id[4] = 0xfe;
	id[5] = mac[3];
	id[6] = mac[4];
	id[7] = mac[5];

	DSDEF(ppi)->numberPorts = 1;
	pp_memcpy(&DSDEF(ppi)->clockQuality, &OPTS(ppi)->clock_quality,
		sizeof(ClockQuality));
	DSDEF(ppi)->priority1 = OPTS(ppi)->prio1;
	DSDEF(ppi)->priority2 = OPTS(ppi)->prio2;
	DSDEF(ppi)->domainNumber = OPTS(ppi)->domain_number;
	DSDEF(ppi)->slaveOnly = OPTS(ppi)->slave_only;
	if (OPTS(ppi)->slave_only)
		ppi->defaultDS->clockQuality.clockClass = 255;

	/* Initialize port data set */
	pp_memcpy(ppi->portDS->portIdentity.clockIdentity,
		ppi->defaultDS->clockIdentity, PP_CLOCK_IDENTITY_LENGTH);
	DSPOR(ppi)->portIdentity.portNumber = 1;
	DSPOR(ppi)->logMinDelayReqInterval = PP_DEFAULT_DELAYREQ_INTERVAL;
	DSPOR(ppi)->peerMeanPathDelay.seconds = 0;
	DSPOR(ppi)->peerMeanPathDelay.nanoseconds = 0;
	DSPOR(ppi)->logAnnounceInterval = OPTS(ppi)->announce_intvl;
	DSPOR(ppi)->announceReceiptTimeout =
		PP_DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
	DSPOR(ppi)->logSyncInterval = OPTS(ppi)->sync_intvl;
	DSPOR(ppi)->delayMechanism = PP_DEFAULT_DELAY_MECHANISM;
	DSPOR(ppi)->logMinPdelayReqInterval = PP_DEFAULT_PDELAYREQ_INTERVAL;
	DSPOR(ppi)->versionNumber = PP_VERSION_PTP;

	/* FIXME: should be autodetected */
#ifdef PPSI_SLAVE
	DSPOR(ppi)->wrConfig = WR_S_ONLY;
#else

	int lock_timeout, start_tics;
	start_tics = timer_get_tics();
	DSPOR(ppi)->wrConfig = WR_M_ONLY;
	spll_init(2, 0, 1); /* SPLL_MODE_FREE_RUNNING_MASTER */
	pps_gen_enable_output(0);
	lock_timeout = LOCK_TIMEOUT_GM;

	PP_PRINTF("Locking...");
	while(!spll_check_lock(0) && lock_timeout)
	{
		timer_delay(1000);
		if(timer_get_tics() - start_tics > lock_timeout)
		{
			PP_PRINTF("\nLocking timeout.\n");
		}
	}
	PP_PRINTF("\nLocking end.\n");
	pps_gen_enable_output(1);
#endif
	DSPOR(ppi)->wrStateTimeout = WR_DEFAULT_STATE_TIMEOUT_MS;
	DSPOR(ppi)->wrStateRetry = WR_DEFAULT_STATE_REPEAT;
	DSPOR(ppi)->calPeriod = WR_DEFAULT_CAL_PERIOD;
	DSPOR(ppi)->wrModeOn = 0;
	DSPOR(ppi)->parentWrConfig = 0;

	if (pp_timer_init(ppi))
		goto failure;

	pp_init_clock(ppi);
	wr_servo_reset();

	m1(ppi);

	msg_pack_header(ppi, ppi->buf_out);

	ppi->next_state = PPS_LISTENING;
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;

failure:
	pp_printf("Failed to initialize network\n");
	ppi->next_state = PPS_FAULTY;
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;
}
