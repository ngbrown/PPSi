/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

/* Flag Field bits symbolic names (table 57, pag. 151) */
#define FFB_LI61	0x01
#define FFB_LI59	0x02
#define FFB_UTCV	0x04
#define FFB_PTP		0x08
#define FFB_TTRA	0x10
#define FFB_FTRA	0x20

/* ppi->port_idx port is becoming Master. Table 13 (9.3.5) of the spec. */
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


/* ppi->port_idx port is synchronized to Ebest Table 16 (9.3.5) of the spec. */
static void s1(struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann)
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
	if (prop->currentUtcOffset != ann->currentUtcOffset) {
		pp_diag(ppi, bmc, 1, "New UTC offset: %i\n",
			ann->currentUtcOffset);
		prop->currentUtcOffset = ann->currentUtcOffset;
		ppi->t_ops->set(ppi, NULL);
	}

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

static void p1(struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann)
{
	/* In the default implementation, nothing should be done when a port goes
	 * to passive state. This empty function is a placeholder for
	 * extension-specific needs, to be implemented as a hook */
}

/* Copy local data set into header and ann message. 9.3.4 table 12. */
static void copy_d0(struct pp_instance *ppi, struct pp_frgn_master *m)
{
	struct DSDefault *defds = DSDEF(ppi);
	struct MsgHeader *hdr = &m->hdr;
	struct MsgAnnounce *ann = &m->ann;

	ann->grandmasterIdentity = defds->clockIdentity;
	ann->grandmasterClockQuality = defds->clockQuality;
	ann->grandmasterPriority1 = defds->priority1;
	ann->grandmasterPriority2 = defds->priority2;
	ann->stepsRemoved = 0;
	hdr->sourcePortIdentity.clockIdentity = defds->clockIdentity;
}

static int idcmp(struct ClockIdentity *a, struct ClockIdentity *b)
{
	return memcmp(a, b, sizeof(*a));
}

/*
 * Data set comparison between two foreign masters. Return similar to
 * memcmp().  However, lower values take precedence, so in A-B (like
 * in comparisons,   > 0 means B wins (and < 0 means A wins).
 */
static int bmc_dataset_cmp(struct pp_instance *ppi,
			   struct pp_frgn_master *a,
			   struct pp_frgn_master *b)
{
	struct ClockQuality *qa, *qb;
	struct MsgAnnounce *aa = &a->ann;
	struct MsgAnnounce *ab = &b->ann;
	struct ClockIdentity *ida = &a->hdr.sourcePortIdentity.clockIdentity;
	struct ClockIdentity *idb = &b->hdr.sourcePortIdentity.clockIdentity;
	struct ClockIdentity *idparent;
	int diff;

	/* dataset_cmp is called several times, so report only at level 2 */
	pp_diag(ppi, bmc, 2,"%s\n", __func__);

	if (!idcmp(&aa->grandmasterIdentity, &ab->grandmasterIdentity)) {

		/* The grandmaster is the same: part 2, fig 28, page 90. */

		diff = aa->stepsRemoved - ab->stepsRemoved;
		if (diff > 1 || diff < -1)
			return diff;

		idparent = &DSPAR(ppi)->parentPortIdentity.clockIdentity;

		if (diff > 0) {
			if (!idcmp(ida, idparent)) {
				pp_diag(ppi, bmc, 1,"%s:%i: Error 1\n",
					__func__, __LINE__);
				return 0;
			}
			return 1;

		}
		if (diff < 0) {
			if (!idcmp(idb, idparent)) {
				pp_diag(ppi, bmc, 1,"%s:%i: Error 1\n",
					__func__, __LINE__);
				return 0;
			}
			return -1;
		}
		/* stepsRemoved is equal, compare identities */
		diff = idcmp(ida, idb);
		if (!diff) {
			pp_diag(ppi, bmc, 1,"%s:%i: Error 2\n", __func__, __LINE__);
			return 0;
		}
		return diff;
	}

	/* The grandmasters are different: part 1, fig 27, page 89. */
	qa = &aa->grandmasterClockQuality;
	qb = &ab->grandmasterClockQuality;

	if (aa->grandmasterPriority1 != ab->grandmasterPriority1)
		return aa->grandmasterPriority1 - ab->grandmasterPriority1;

	if (qa->clockClass != qb->clockClass)
		return qa->clockClass - qb->clockClass;

	if (qa->clockAccuracy != qb->clockAccuracy)
		return qa->clockAccuracy - qb->clockAccuracy;

	if (qa->offsetScaledLogVariance != qb->offsetScaledLogVariance)
		return qa->clockClass - qb->clockClass;

	if (aa->grandmasterPriority2 != ab->grandmasterPriority2)
		return aa->grandmasterPriority2 - ab->grandmasterPriority2;

	return idcmp(&aa->grandmasterIdentity, &ab->grandmasterIdentity);
}

/* State decision algorithm 9.3.3 Fig 26 */
static int bmc_state_decision(struct pp_instance *ppi,
							  struct pp_frgn_master *m)
{
	int cmpres;
	struct pp_frgn_master myself;

	if (ppi->role == PPSI_ROLE_SLAVE)
		goto slave;

	if ((!ppi->frgn_rec_num) && (ppi->state == PPS_LISTENING))
		return PPS_LISTENING;

	/* copy local information to a foreign_master structure */
	copy_d0(ppi, &myself);

	/* dataset_cmp is "a - b" but lower values win */
	cmpres = bmc_dataset_cmp(ppi, &myself, m);

	if (ppi->role == PPSI_ROLE_MASTER)
		goto master;
	
	if (DSDEF(ppi)->clockQuality.clockClass < 128) {
		if (cmpres < 0)
			goto master;
		if (cmpres > 0)
			goto passive;
	}
	if (cmpres < 0)
		goto master;
	if (cmpres > 0) {
		if (DSDEF(ppi)->numberPorts == 1)
			goto slave; /* directly skip to ordinary clock handling */
		else
			goto check_boundary_clk;
	}

	pp_diag(ppi, bmc, 1,"%s: error\n", __func__);

	/*  MB: Is this the return code below correct? */
	/*  Anyway, it's a valid return code. */
	return PPS_FAULTY;

check_boundary_clk:
	if (ppi->port_idx == GLBS(ppi)->ebest_idx) /* This port is the Ebest */
		goto slave;

	/* If idcmp returns 0, it means that this port is not the best because
		* Ebest is better by topology than Erbest */
	if (!idcmp(&myself.ann.grandmasterIdentity,
			&m->ann.grandmasterIdentity))
		goto passive;
	else
		goto master;

passive:
	p1(ppi, &m->hdr, &m->ann);
	pp_diag(ppi, bmc, 1,"%s: passive\n", __func__);
	return PPS_PASSIVE;

master:
	//TODO: consider whether a smarter solution is needed for non-simple cases
	if(cmpres < 0) // it is M1 and M2, see IEEE1588-2008, page 87, in short switch is a GM
		m1(ppi); //GM
	pp_diag(ppi, bmc, 1,"%s: master\n", __func__);
	return PPS_MASTER;

slave:
	s1(ppi, &m->hdr, &m->ann);
	pp_diag(ppi, bmc, 1,"%s: slave\n", __func__);
	return PPS_SLAVE;

}

/* Find Ebest, 9.3.2.2 */
void bmc_update_ebest(struct pp_globals *ppg)
{
	int i, best;
	struct pp_instance *ppi, *ppi_best;

	for (i = 1, best = 0; i < ppg->defaultDS->numberPorts; i++) {

		ppi_best = INST(ppg, best);
		ppi = INST(ppg, i);

		if ((ppi->frgn_rec_num > 0) &&
			 (bmc_dataset_cmp(ppi,
				&ppi_best->frgn_master[ppi_best->frgn_rec_best],
				&ppi->frgn_master[ppi->frgn_rec_best])
				< 0))
				best = i;
	}

	if (ppg->ebest_idx != best) {
		ppg->ebest_idx = best;
		ppg->ebest_updated = 1;
	}
}

int bmc(struct pp_instance *ppi)
{
	struct pp_frgn_master *frgn_master = ppi->frgn_master;
	int i, best;

	if (!ppi->frgn_rec_num)
		if (ppi->state == PPS_MASTER)	{
			m1(ppi);
			return ppi->state;
		}

	/* Find Erbest, 9.3.2.3 */
	for (i = 1, best = 0; i < ppi->frgn_rec_num; i++)
		if (bmc_dataset_cmp(ppi, &frgn_master[i], &frgn_master[best])
		    < 0)
			best = i;

	pp_diag(ppi, bmc, 1,"Best foreign master is %i/%i\n", best,
		ppi->frgn_rec_num);
	if (ppi->frgn_rec_best != best) {
		ppi->frgn_rec_best = best;
		bmc_update_ebest(GLBS(ppi));
	}

	return bmc_state_decision(ppi, &frgn_master[best]);
}
