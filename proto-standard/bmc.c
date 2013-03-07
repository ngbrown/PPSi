/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>

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
	struct DSParent *parent = DSPAR(ppi);
	struct DSDefault *defds = DSDEF(ppi);

	/* Current data set update */
	DSCUR(ppi)->stepsRemoved = 0;
	clear_TimeInternal(&DSCUR(ppi)->offsetFromMaster);
	clear_TimeInternal(&DSCUR(ppi)->meanPathDelay);

	/* Parent data set: we are the parent */
	memset(parent, 0, sizeof(*parent));
	parent->parentPortIdentity.clockIdentity = defds->clockIdentity;
	/* FIXME: the port? */

	/* Copy grandmaster params from our defds (FIXME: is ir right?) */
	parent->grandmasterIdentity = defds->clockIdentity;
	parent->grandmasterClockQuality = defds->clockQuality;
	parent->grandmasterPriority1 = defds->priority1;
	parent->grandmasterPriority2 = defds->priority2;

	/* Time Properties data set */
	DSPRO(ppi)->timeSource = INTERNAL_OSCILLATOR;
}


/* Local clock is synchronized to Ebest Table 16 (9.3.5) of the spec. */
void s1(struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann)
{
	struct DSParent *parent = DSPAR(ppi);
	struct DSTimeProperties *prop = DSPRO(ppi);

	/* Current DS */
	DSCUR(ppi)->stepsRemoved = ann->stepsRemoved + 1;

	/* Parent DS */
	parent->parentPortIdentity = hdr->sourcePortIdentity;
	parent->grandmasterIdentity = ann->grandmasterIdentity;
	parent->grandmasterClockQuality = ann->grandmasterClockQuality;
	parent->grandmasterPriority1 = ann->grandmasterPriority1;
	parent->grandmasterPriority2 = ann->grandmasterPriority2;

	/* Timeproperties DS */
	prop->timeSource = ann->timeSource;
	prop->currentUtcOffset = ann->currentUtcOffset;

	/* FIXME: can't we just copy the bit keeping values? */
	prop->currentUtcOffsetValid = ((hdr->flagField[1] & FFB_UTCV)	!= 0);
	prop->leap59 = ((hdr->flagField[1] & FFB_LI59) != 0);
	prop->leap61 = ((hdr->flagField[1] & FFB_LI61) != 0);
	prop->timeTraceable = ((hdr->flagField[1] & FFB_TTRA) != 0);
	prop->frequencyTraceable = ((hdr->flagField[1] & FFB_FTRA) != 0);
	prop->ptpTimescale = ((hdr->flagField[1] & FFB_PTP) != 0);

	if (pp_hooks.s1)
		pp_hooks.s1(ppi, hdr, ann);
}


/* Copy local data set into header and ann message. 9.3.4 table 12. */
static void copy_d0( struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann)
{
	struct DSDefault *defds = DSDEF(ppi);

	ann->grandmasterIdentity = defds->clockIdentity;
	ann->grandmasterClockQuality = defds->clockQuality;
	ann->grandmasterPriority1 = defds->priority1;
	ann->grandmasterPriority2 = defds->priority2;
	ann->stepsRemoved = 0;
	hdr->sourcePortIdentity.clockIdentity = defds->clockIdentity;
}


/*
 * Data set comparison between two foreign masters (9.3.4 fig 27)
 * return similar to memcmp()
 */
static int bmc_dataset_cmp(struct pp_instance *ppi,
			 MsgHeader *hdr_a, MsgAnnounce *ann_a,
			 MsgHeader *hdr_b, MsgAnnounce *ann_b)
{
	int comp = 0;
	Octet *ppci;

	PP_VPRINTF("BMC: in bmc_dataset_cmp\n");

	/* Identity comparison */
	if (!memcmp(&ann_a->grandmasterIdentity,
		       &ann_b->grandmasterIdentity, PP_CLOCK_IDENTITY_LENGTH)) {

		/* Algorithm part2 Fig 28 */
		if (ann_a->stepsRemoved > ann_b->stepsRemoved + 1)
			return 1;

		else if (ann_b->stepsRemoved > ann_a->stepsRemoved + 1)
			return -1;

		else { /* A within 1 of B */

			ppci = DSPAR(ppi)->parentPortIdentity.clockIdentity.id;

			if (ann_a->stepsRemoved > ann_b->stepsRemoved) {
				if (!memcmp(
					&hdr_a->sourcePortIdentity.clockIdentity,
					ppci,
					PP_CLOCK_IDENTITY_LENGTH)) {
					PP_PRINTF("Sender=Receiver: Error -1");
					return 0;
				} else
					return 1;

			} else if (ann_b->stepsRemoved > ann_a->stepsRemoved) {
				if (!memcmp(
					&hdr_b->sourcePortIdentity.clockIdentity,
					ppci,
					PP_CLOCK_IDENTITY_LENGTH)) {
					PP_PRINTF("Sender=Receiver: Error -3");
					return 0;
				} else {
					return -1;
				}
			} else { /* steps removed A == steps removed B */
				if (!memcmp(
					&hdr_a->sourcePortIdentity.clockIdentity,
					&hdr_b->sourcePortIdentity.clockIdentity,
					PP_CLOCK_IDENTITY_LENGTH)) {
					PP_PRINTF("Sender=Receiver: Error -2");
					return 0;
				} else if ((memcmp(
					&hdr_a->sourcePortIdentity.clockIdentity,
					&hdr_b->sourcePortIdentity.clockIdentity,
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
							comp = memcmp(&ann_a->grandmasterIdentity, &ann_b->grandmasterIdentity, PP_CLOCK_IDENTITY_LENGTH);
							if (comp < 0)
								return -1;
							else if (comp > 0)
								return 1;
							else
								return 0;
						} else {
						/* Priority2 are not identical */
							comp = memcmp(&ann_a->grandmasterPriority2, &ann_b->grandmasterPriority2, 1);
							if (comp < 0)
								return -1;
							else if (comp > 0)
								return 1;
							else
								return 0;
						}
					} else {
						/* offsetScaledLogVariance are not identical */
						comp = memcmp(&ann_a->grandmasterClockQuality.clockClass, &ann_b->grandmasterClockQuality.clockClass, 1);
						if (comp < 0)
							return -1;
						else if (comp > 0)
							return 1;
						else
							return 0;
					}

				} else { /*  Accuracy are not identitcal */
					comp = memcmp(&ann_a->grandmasterClockQuality.clockAccuracy, &ann_b->grandmasterClockQuality.clockAccuracy, 1);
					if (comp < 0)
						return -1;
					else if (comp > 0)
						return 1;
					else
						return 0;
				}
			} else { /* ClockClass are not identical */
				comp = memcmp(&ann_a->grandmasterClockQuality.clockClass, &ann_b->grandmasterClockQuality.clockClass, 1);
				if (comp < 0)
					return -1;
				else if (comp > 0)
					return 1;
				else
					return 0;
			}
		} else { /*  Priority1 are not identical */
			comp = memcmp(&ann_a->grandmasterPriority1, &ann_b->grandmasterPriority1, 1);
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
UInteger8 bmc_state_decision(struct pp_instance *ppi,
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

	copy_d0(ppi, &ppi->received_ptp_header, &ppi->msg_tmp.announce);


	cmpres = bmc_dataset_cmp(ppi,
				 &ppi->received_ptp_header,
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

	if (cmpres == 0)
		PP_PRINTF("Error in bmc_state_decision, cmpres=0.\n");

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
