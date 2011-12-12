/*
 * FIXME header
 */
#include <pproto/pproto.h>
#include <dep/dep.h>



/* Local clock is becoming Master. Table 13 (9.3.5) of the spec. */
void m1(struct pp_instance *ppi)
{
	/* Current data set update */
	DSCUR(ppi)->stepsRemoved = 0;
	DSCUR(ppi)->offsetFromMaster.nanoseconds = 0;
	DSCUR(ppi)->offsetFromMaster.seconds = 0;
	DSCUR(ppi)->meanPathDelay.nanoseconds = 0;
	DSCUR(ppi)->meanPathDelay.seconds = 0;

	/* Parent data set */
	pp_memcpy(DSPAR(ppi)->parentPortIdentity.clockIdentity,
		  DSDEF(ppi)->clockIdentity,PP_CLOCK_IDENTITY_LENGTH);
	DSPAR(ppi)->parentPortIdentity.portNumber = 0;
	DSPAR(ppi)->parentStats = PP_DEFAULT_PARENTS_STATS;
	DSPAR(ppi)->observedParentClockPhaseChangeRate = 0;
	DSPAR(ppi)->observedParentOffsetScaledLogVariance = 0;
	pp_memcpy(DSPAR(ppi)->grandmasterIdentity, DSDEF(ppi)->clockIdentity,
		  PP_CLOCK_IDENTITY_LENGTH);
	DSPAR(ppi)->grandmasterClockQuality.clockAccuracy =
		DSDEF(ppi)->clockQuality.clockAccuracy;
	DSPAR(ppi)->grandmasterClockQuality.clockClass =
		DSDEF(ppi)->clockQuality.clockClass;
	DSPAR(ppi)->grandmasterClockQuality.offsetScaledLogVariance =
		DSDEF(ppi)->clockQuality.offsetScaledLogVariance;
	DSPAR(ppi)->grandmasterPriority1 = DSDEF(ppi)->priority1;
	DSPAR(ppi)->grandmasterPriority2 = DSDEF(ppi)->priority2;

	/* Time Properties data set */
	DSPRO(ppi)->timeSource = INTERNAL_OSCILLATOR;
}


/* Local clock is synchronized to Ebest Table 16 (9.3.5) of the spec. */
void s1(MsgHeader *header, MsgAnnounce *announce, struct pp_instance *ppi)
{
	/* Current DS */
	DSCUR(ppi)->stepsRemoved = announce->stepsRemoved + 1;

	/* Parent DS */
	pp_memcpy(DSPAR(ppi)->parentPortIdentity.clockIdentity,
	       header->sourcePortIdentity.clockIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	DSPAR(ppi)->parentPortIdentity.portNumber =
		header->sourcePortIdentity.portNumber;

	pp_memcpy(DSPAR(ppi)->grandmasterIdentity,
		  announce->grandmasterIdentity, PP_CLOCK_IDENTITY_LENGTH);

	DSPAR(ppi)->grandmasterClockQuality.clockAccuracy =
		announce->grandmasterClockQuality.clockAccuracy;
	DSPAR(ppi)->grandmasterClockQuality.clockClass =
		announce->grandmasterClockQuality.clockClass;
	DSPAR(ppi)->grandmasterClockQuality.offsetScaledLogVariance =
		announce->grandmasterClockQuality.offsetScaledLogVariance;
	DSPAR(ppi)->grandmasterPriority1 = announce->grandmasterPriority1;
	DSPAR(ppi)->grandmasterPriority2 = announce->grandmasterPriority2;

	/* Timeproperties DS */
	DSPRO(ppi)->currentUtcOffset = announce->currentUtcOffset;
        /* "Valid" is bit 2 in second octet of flagfield */
	DSPRO(ppi)->currentUtcOffsetValid = ((header->flagField[1] & 0x04) ==
					   0x04);

	DSPRO(ppi)->leap59 = ((header->flagField[1] & 0x02) == 0x02);
	DSPRO(ppi)->leap61 = ((header->flagField[1] & 0x01) == 0x01);
	DSPRO(ppi)->timeTraceable = ((header->flagField[1] & 0x10) == 0x10);
	DSPRO(ppi)->frequencyTraceable = ((header->flagField[1] & 0x20)
					   == 0x20);
	DSPRO(ppi)->ptpTimescale = ((header->flagField[1] & 0x08) == 0x08);
	DSPRO(ppi)->timeSource = announce->timeSource;
}


/* Copy local data set into header and announce message. 9.3.4 table 12. */
void copy_d0(MsgHeader *header, MsgAnnounce *announce, struct pp_instance *ppi)
{
	announce->grandmasterPriority1 = DSDEF(ppi)->priority1;
	pp_memcpy(announce->grandmasterIdentity, DSDEF(ppi)->clockIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	announce->grandmasterClockQuality.clockClass =
		DSDEF(ppi)->clockQuality.clockClass;
	announce->grandmasterClockQuality.clockAccuracy =
		DSDEF(ppi)->clockQuality.clockAccuracy;
	announce->grandmasterClockQuality.offsetScaledLogVariance =
		DSDEF(ppi)->clockQuality.offsetScaledLogVariance;
	announce->grandmasterPriority2 = DSDEF(ppi)->priority2;
	announce->stepsRemoved = 0;
	pp_memcpy(header->sourcePortIdentity.clockIdentity,
	       DSDEF(ppi)->clockIdentity, PP_CLOCK_IDENTITY_LENGTH);
}


/*
 * Data set comparison bewteen two foreign masters (9.3.4 fig 27)
 * return similar to memcmp()
 */
Integer8 bmc_dataset_cmp(MsgHeader *hdr_a, MsgAnnounce *ann_a,
		     MsgHeader *hdr_b,MsgAnnounce *ann_b,
		     struct pp_instance *ppi)
{
	/* FIXME DBGV("Data set comparison \n"); */
	short comp = 0;
	Octet *ppci;

	/* Identity comparison */
	if (!pp_memcmp(ann_a->grandmasterIdentity,
		       ann_b->grandmasterIdentity,PP_CLOCK_IDENTITY_LENGTH)) {

		/* Algorithm part2 Fig 28 */
		if (ann_a->stepsRemoved > ann_b->stepsRemoved + 1)
			return 1;

		else if (ann_b->stepsRemoved > ann_a->stepsRemoved + 1)
			return -1;

		else { /* A within 1 of B */

			ppci = DSPAR(ppi)->parentPortIdentity.clockIdentity;

			if (ann_a->stepsRemoved > ann_b->stepsRemoved) {
				if (!pp_memcmp(
					hdr_a->sourcePortIdentity.clockIdentity,
					ppci,
					PP_CLOCK_IDENTITY_LENGTH)) {
					/* FIXME
					 * DBG("Sender=Receiver : Error -1");
					 */
					return 0;
				} else
					return 1;

			} else if (ann_b->stepsRemoved > ann_a->stepsRemoved) {
				if (!pp_memcmp(
					hdr_b->sourcePortIdentity.clockIdentity,
					ppci,
					PP_CLOCK_IDENTITY_LENGTH)) {
					/* FIXME
					 * DBG("Sender=Receiver : Error -1");
					 */
					return 0;
				} else {
					return -1;
				}
			}
			else { /* steps removed A == steps removed B */
				if (!pp_memcmp(
					hdr_a->sourcePortIdentity.clockIdentity,
					hdr_b->sourcePortIdentity.clockIdentity,
					PP_CLOCK_IDENTITY_LENGTH)) {
					/* FIXME
					 * DBG("Sender=Receiver : Error -2");
					 */
					return 0;
				} else if ((pp_memcmp(
					hdr_a->sourcePortIdentity.clockIdentity,
					hdr_b->sourcePortIdentity.clockIdentity,
					PP_CLOCK_IDENTITY_LENGTH)) < 0)
					return -1;
				else
					return 1;
			}
		}
	} else { /* GrandMaster are not identical */
		/* FIXME: rewrite in a more readable way */
		if(ann_a->grandmasterPriority1 == ann_b->grandmasterPriority1) {
			if (ann_a->grandmasterClockQuality.clockClass ==
			    ann_b->grandmasterClockQuality.clockClass) {
				if (ann_a->grandmasterClockQuality.clockAccuracy == ann_b->grandmasterClockQuality.clockAccuracy) {
					if (ann_a->grandmasterClockQuality.offsetScaledLogVariance == ann_b->grandmasterClockQuality.offsetScaledLogVariance) {
						if (ann_a->grandmasterPriority2 == ann_b->grandmasterPriority2) {
							comp = pp_memcmp(ann_a->grandmasterIdentity,ann_b->grandmasterIdentity,PP_CLOCK_IDENTITY_LENGTH);
							if (comp < 0)
								return -1;
							else if (comp > 0)
								return 1;
							else
								return 0;
						} else {
						/* Priority2 are not identical */
							comp =pp_memcmp(&ann_a->grandmasterPriority2,&ann_b->grandmasterPriority2,1);
							if (comp < 0)
								return -1;
							else if (comp > 0)
								return 1;
							else
								return 0;
						}
					} else {
						/* offsetScaledLogVariance are not identical */
						comp= pp_memcmp(&ann_a->grandmasterClockQuality.clockClass,&ann_b->grandmasterClockQuality.clockClass,1);
						if (comp < 0)
							return -1;
						else if (comp > 0)
							return 1;
						else
							return 0;
					}

				} else { /*  Accuracy are not identitcal */
					comp = pp_memcmp(&ann_a->grandmasterClockQuality.clockAccuracy,&ann_b->grandmasterClockQuality.clockAccuracy,1);
					if (comp < 0)
						return -1;
					else if (comp > 0)
						return 1;
					else
						return 0;
				}
			} else { /* ClockClass are not identical */
				comp =  pp_memcmp(&ann_a->grandmasterClockQuality.clockClass,&ann_b->grandmasterClockQuality.clockClass,1);
				if (comp < 0)
					return -1;
				else if (comp > 0)
					return 1;
				else
					return 0;
			}
		} else { /*  Priority1 are not identical */
			comp =  pp_memcmp(&ann_a->grandmasterPriority1,&ann_b->grandmasterPriority1,1);
			if (comp < 0)
				return -1;
			else if (comp > 0)
				return 1;
			else
				return 0;
		}
	}
}

/* State decision algorithm 9.3.3 Fig 26 */
UInteger8 bmc_state_decision(MsgHeader *hdr, MsgAnnounce *ann,
		 struct pp_runtime_opts *rt_opts, struct pp_instance *ppi)
{
	int cmpres;

	if (rt_opts->slave_only) {
		s1(hdr, ann, ppi);
		return PPS_SLAVE;
	}

	if ((!ppi->number_foreign_records) && (ppi->state == PPS_LISTENING))
		return PPS_LISTENING;

	copy_d0(&ppi->msg_tmp_header, &ppi->msg_tmp.announce, ppi);


	cmpres = bmc_dataset_cmp(&ppi->msg_tmp_header,
				 &ppi->msg_tmp.announce,
				 hdr,ann,ppi);

	if (DSDEF(ppi)->clockQuality.clockClass < 128) {

		if (cmpres < 0) {
			m1(ppi);
			return PPS_MASTER;
		} else if (cmpres > 0) {
			s1(hdr, ann, ppi);
			return PPS_PASSIVE;
		} else {
			/* FIXME DBG("Error in bmcDataSetComparison..\n"); */
		}

	} else {

		if (cmpres < 0) {
			m1(ppi);
			return PPS_MASTER;
		} else if (cmpres > 0) {
			s1(hdr,ann,ppi);
			return PPS_SLAVE;
		} else {
			/* FIXME DBG("Error in bmcDataSetComparison..\n"); */
		}

	}

	/*  MB: Is this the return code below correct? */
	/*  Anyway, it's a valid return code. */
	return PPS_FAULTY;
}



UInteger8 bmc(struct pp_frgn_master *frgn_master,
	      struct pp_runtime_opts *rt_opts, struct pp_instance *ppi)
{
	Integer16 i, best;

	if (!ppi->number_foreign_records)
		if (ppi->state == PPS_MASTER)	{
			m1(ppi);
			return ppi->state;
		}

	for (i=1, best = 0; i < ppi->number_foreign_records; i++)
		if ((bmc_dataset_cmp(&frgn_master[i].hdr,
				     &frgn_master[i].ann,
				     &frgn_master[best].hdr,
				     &frgn_master[best].ann,
				     ppi)) < 0)
			best = i;

	/* FIXME DBGV("Best record : %d \n",best); */
	ppi->foreign_record_best = best;

	return (bmc_state_decision(&frgn_master[best].hdr,
				   &frgn_master[best].ann,
				   rt_opts,ppi));
}
