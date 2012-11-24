/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "wr-api.h"

/* Flag Field bits symbolic names (table 57, pag. 151) */
#define FFB_LI61	0x01
#define FFB_LI59	0x02
#define FFB_UTCV	0x04
#define FFB_PTP		0x08
#define FFB_TTRA	0x10
#define FFB_FTRA	0x20

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
		  DSDEF(ppi)->clockIdentity, PP_CLOCK_IDENTITY_LENGTH);
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
void s1(struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann)
{
	/* Current DS */
	DSCUR(ppi)->stepsRemoved = ann->stepsRemoved + 1;

	/* Parent DS */
	pp_memcpy(DSPAR(ppi)->parentPortIdentity.clockIdentity,
		hdr->sourcePortIdentity.clockIdentity,
		PP_CLOCK_IDENTITY_LENGTH);
	DSPAR(ppi)->parentPortIdentity.portNumber =
		hdr->sourcePortIdentity.portNumber;

	pp_memcpy(DSPAR(ppi)->grandmasterIdentity,
		ann->grandmasterIdentity, PP_CLOCK_IDENTITY_LENGTH);

	DSPAR(ppi)->grandmasterClockQuality.clockAccuracy =
		ann->grandmasterClockQuality.clockAccuracy;
	DSPAR(ppi)->grandmasterClockQuality.clockClass =
		ann->grandmasterClockQuality.clockClass;
	DSPAR(ppi)->grandmasterClockQuality.offsetScaledLogVariance =
		ann->grandmasterClockQuality.offsetScaledLogVariance;
	DSPAR(ppi)->grandmasterPriority1 = ann->grandmasterPriority1;
	DSPAR(ppi)->grandmasterPriority2 = ann->grandmasterPriority2;

	/* Timeproperties DS */
	DSPRO(ppi)->currentUtcOffset = ann->currentUtcOffset;
	/* "Valid" is bit 2 in second octet of flagfield */
	DSPRO(ppi)->currentUtcOffsetValid = ((hdr->flagField[1] & FFB_UTCV)
		!= 0);

	DSPRO(ppi)->leap59 = ((hdr->flagField[1] & FFB_LI59) != 0);
	DSPRO(ppi)->leap61 = ((hdr->flagField[1] & FFB_LI61) != 0);
	DSPRO(ppi)->timeTraceable = ((hdr->flagField[1] & FFB_TTRA) != 0);
	DSPRO(ppi)->frequencyTraceable = ((hdr->flagField[1] & FFB_FTRA) != 0);
	DSPRO(ppi)->ptpTimescale = ((hdr->flagField[1] & FFB_PTP) != 0);
	DSPRO(ppi)->timeSource = ann->timeSource;

	/* White Rabbit */
	DSPOR(ppi)->parentIsWRnode = ((ann->wrFlags & WR_NODE_MODE) != NON_WR);
	DSPOR(ppi)->parentWrModeOn =
		(ann->wrFlags & WR_IS_WR_MODE) ? TRUE : FALSE;
	DSPOR(ppi)->parentCalibrated =
			((ann->wrFlags & WR_IS_CALIBRATED) ? 1 : 0);
	DSPOR(ppi)->parentWrConfig = ann->wrFlags & WR_NODE_MODE;
	DSCUR(ppi)->primarySlavePortNumber =
		DSPOR(ppi)->portIdentity.portNumber;

}


/* Copy local data set into header and ann message. 9.3.4 table 12. */
void copy_d0( struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann)
{
	ann->grandmasterPriority1 = DSDEF(ppi)->priority1;
	pp_memcpy(ann->grandmasterIdentity, DSDEF(ppi)->clockIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	ann->grandmasterClockQuality.clockClass =
		DSDEF(ppi)->clockQuality.clockClass;
	ann->grandmasterClockQuality.clockAccuracy =
		DSDEF(ppi)->clockQuality.clockAccuracy;
	ann->grandmasterClockQuality.offsetScaledLogVariance =
		DSDEF(ppi)->clockQuality.offsetScaledLogVariance;
	ann->grandmasterPriority2 = DSDEF(ppi)->priority2;
	ann->stepsRemoved = 0;
	pp_memcpy(hdr->sourcePortIdentity.clockIdentity,
	       DSDEF(ppi)->clockIdentity, PP_CLOCK_IDENTITY_LENGTH);
}


/*
 * Data set comparison bewteen two foreign masters (9.3.4 fig 27)
 * return similar to memcmp()
 */
Integer8 bmc_dataset_cmp(struct pp_instance *ppi,
			 MsgHeader *hdr_a, MsgAnnounce *ann_a,
			 MsgHeader *hdr_b, MsgAnnounce *ann_b)
{
	short comp = 0;
	Octet *ppci;

	PP_VPRINTF("BMC: in bmc_dataset_cmp\n");

	/* Identity comparison */
	if (!pp_memcmp(ann_a->grandmasterIdentity,
		       ann_b->grandmasterIdentity, PP_CLOCK_IDENTITY_LENGTH)) {

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
					PP_PRINTF("Sender=Receiver: Error -1");
					return 0;
				} else
					return 1;

			} else if (ann_b->stepsRemoved > ann_a->stepsRemoved) {
				if (!pp_memcmp(
					hdr_b->sourcePortIdentity.clockIdentity,
					ppci,
					PP_CLOCK_IDENTITY_LENGTH)) {
					PP_PRINTF("Sender=Receiver: Error -3");
					return 0;
				} else {
					return -1;
				}
			} else { /* steps removed A == steps removed B */
				if (!pp_memcmp(
					hdr_a->sourcePortIdentity.clockIdentity,
					hdr_b->sourcePortIdentity.clockIdentity,
					PP_CLOCK_IDENTITY_LENGTH)) {
					PP_PRINTF("Sender=Receiver: Error -2");
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
		if (ann_a->grandmasterPriority1 == ann_b->grandmasterPriority1) {
			if (ann_a->grandmasterClockQuality.clockClass ==
			    ann_b->grandmasterClockQuality.clockClass) {
				if (ann_a->grandmasterClockQuality.clockAccuracy == ann_b->grandmasterClockQuality.clockAccuracy) {
					if (ann_a->grandmasterClockQuality.offsetScaledLogVariance == ann_b->grandmasterClockQuality.offsetScaledLogVariance) {
						if (ann_a->grandmasterPriority2 == ann_b->grandmasterPriority2) {
							comp = pp_memcmp(ann_a->grandmasterIdentity, ann_b->grandmasterIdentity, PP_CLOCK_IDENTITY_LENGTH);
							if (comp < 0)
								return -1;
							else if (comp > 0)
								return 1;
							else
								return 0;
						} else {
						/* Priority2 are not identical */
							comp = pp_memcmp(&ann_a->grandmasterPriority2, &ann_b->grandmasterPriority2, 1);
							if (comp < 0)
								return -1;
							else if (comp > 0)
								return 1;
							else
								return 0;
						}
					} else {
						/* offsetScaledLogVariance are not identical */
						comp = pp_memcmp(&ann_a->grandmasterClockQuality.clockClass, &ann_b->grandmasterClockQuality.clockClass, 1);
						if (comp < 0)
							return -1;
						else if (comp > 0)
							return 1;
						else
							return 0;
					}

				} else { /*  Accuracy are not identitcal */
					comp = pp_memcmp(&ann_a->grandmasterClockQuality.clockAccuracy, &ann_b->grandmasterClockQuality.clockAccuracy, 1);
					if (comp < 0)
						return -1;
					else if (comp > 0)
						return 1;
					else
						return 0;
				}
			} else { /* ClockClass are not identical */
				comp = pp_memcmp(&ann_a->grandmasterClockQuality.clockClass, &ann_b->grandmasterClockQuality.clockClass, 1);
				if (comp < 0)
					return -1;
				else if (comp > 0)
					return 1;
				else
					return 0;
			}
		} else { /*  Priority1 are not identical */
			comp = pp_memcmp(&ann_a->grandmasterPriority1, &ann_b->grandmasterPriority1, 1);
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
UInteger8 bmc_state_decision( struct pp_instance *ppi,
			      MsgHeader *hdr, MsgAnnounce *ann)
{
	int cmpres;

	if (OPTS(ppi)->master_only) {
		m1(ppi);
		return PPS_MASTER;
	}

	if (OPTS(ppi)->slave_only) {
		s1(ppi, hdr, ann);
		return PPS_SLAVE;
	}

	if ((!ppi->number_foreign_records) && (ppi->state == PPS_LISTENING))
		return PPS_LISTENING;

	copy_d0(ppi, &ppi->msg_tmp_header, &ppi->msg_tmp.announce);


	cmpres = bmc_dataset_cmp(ppi,
				 &ppi->msg_tmp_header,
				 &ppi->msg_tmp.announce,
				 hdr, ann);

	if (DSDEF(ppi)->clockQuality.clockClass < 128) {
		if (cmpres < 0) {
			m1(ppi);
			return PPS_MASTER;
		} else if (cmpres > 0) {
			s1(ppi, hdr, ann);
			return PPS_PASSIVE;
		}

	} else {
		if (cmpres < 0) {
			m1(ppi);
			return PPS_MASTER;
		} else if (cmpres > 0) {
			s1(ppi, hdr, ann);
			return PPS_SLAVE;
		}
	}

	if (cmpres == 0) {
		PP_PRINTF("Error in bmc_state_decision, cmpres=0.\n");
	}

	/*  MB: Is this the return code below correct? */
	/*  Anyway, it's a valid return code. */
	return PPS_FAULTY;
}



UInteger8 bmc(struct pp_instance *ppi, struct pp_frgn_master *frgn_master)
{
	Integer16 i, best;

	if (!ppi->number_foreign_records)
		if (ppi->state == PPS_MASTER)	{
			m1(ppi);
			return ppi->state;
		}

	for (i = 1, best = 0; i < ppi->number_foreign_records; i++)
		if (bmc_dataset_cmp(ppi,
				     &frgn_master[i].hdr,
				     &frgn_master[i].ann,
				     &frgn_master[best].hdr,
				     &frgn_master[best].ann) < 0)
			best = i;

	PP_VPRINTF("bmc, best record : %d\n", best);
	ppi->foreign_record_best = best;

	return bmc_state_decision(ppi, &frgn_master[best].hdr,
				   &frgn_master[best].ann);
}
