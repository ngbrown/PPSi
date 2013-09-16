/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

void pp_servo_init(struct pp_instance *ppi)
{
	int d;

	SRV(ppi)->owd_fltr.s_exp = 0;	/* clears one-way delay filter */
	SRV(ppi)->ofm_fltr.s_exp = 0;	/* clears offset-from-master filter */


	if (ppi->t_ops->init_servo) {
		/* The system may pre-set us to keep current frequency */
		d = ppi->t_ops->init_servo(ppi);
		if (d == -1) {
			pp_diag(ppi, servo, 1, "error in t_ops->servo_init");
			d = 0;
		}
		SRV(ppi)->obs_drift = -d; /* note "-" */
	} else {
		/* level clock */
		if (!OPTS(ppi)->no_adjust)
			ppi->t_ops->adjust(ppi, 0, 0);
		SRV(ppi)->obs_drift = 0;
	}

	pp_diag(ppi, servo, 1, "Initialized: obs_drift %i\n",
		SRV(ppi)->obs_drift);
}

/* internal helper, retuerning static storage to be used immediately */
static char *fmt_TI(TimeInternal *t)
{
	static char s[24];

	pp_sprintf(s, "%s%d.%09d",
		(t->seconds < 0 || (t->seconds == 0 && t->nanoseconds < 0))
		   ? "-" : " ",
		   (int)abs(t->seconds), (int)abs(t->nanoseconds));
	return s;
}


/* Called by slave and uncalib when we have t1 and t2 */
void pp_servo_got_sync(struct pp_instance *ppi)
{
	TimeInternal *m_to_s_dly = &SRV(ppi)->m_to_s_dly;

	/*
	 * calc 'master_to_slave_delay', removing the correction field
	 * added by transparent clocks in the path.
	 */
	sub_TimeInternal(m_to_s_dly, &ppi->t2, &ppi->t1);
	sub_TimeInternal(m_to_s_dly, m_to_s_dly, &ppi->cField);
	pp_diag(ppi, servo, 3, "correction field 1: %s\n",
		fmt_TI(&ppi->cField));
}

/*
 * This function makes the necessary checks to discard a set of t1..t4.
 * It relies on owd to be already calculated.
 */
static int pp_servo_bad_event(struct pp_instance *ppi)
{
	TimeInternal *m_to_s_dly = &SRV(ppi)->m_to_s_dly;
	TimeInternal *s_to_m_dly = &SRV(ppi)->s_to_m_dly;
	TimeInternal *owd = &DSCUR(ppi)->oneWayDelay;

	/* Discard one-way delays that overflow a second (makes no sense) */
	if (owd->seconds)
		return 1;

	if (OPTS(ppi)->max_dly) { /* If maxDelay is 0 then it's OFF */
		if (m_to_s_dly->seconds || s_to_m_dly->seconds) {
			pp_diag(ppi, servo, 1,"servo aborted, delay greater "
				"than 1 second\n");
			return 1;
		}
		if (m_to_s_dly->nanoseconds > OPTS(ppi)->max_dly ||
			s_to_m_dly->nanoseconds > OPTS(ppi)->max_dly) {
			pp_diag(ppi, servo, 1, "servo aborted, delay %d or %d "
				"greater than configured maximum %d\n",
			     (int)m_to_s_dly->nanoseconds,
			     (int)s_to_m_dly->nanoseconds,
			     (int)OPTS(ppi)->max_dly);
			return 1;
		}
	}
	return 0;
}


/* called by slave states when delay_resp is received (all t1..t4 are valid) */
void pp_servo_got_resp(struct pp_instance *ppi)
{
	TimeInternal *m_to_s_dly = &SRV(ppi)->m_to_s_dly;
	TimeInternal *s_to_m_dly = &SRV(ppi)->s_to_m_dly;
	TimeInternal *owd = &DSCUR(ppi)->oneWayDelay;
	TimeInternal *ofm = &DSCUR(ppi)->offsetFromMaster;
	struct pp_avg_fltr *owd_fltr = &SRV(ppi)->owd_fltr;
	struct pp_avg_fltr *ofm_fltr = &SRV(ppi)->ofm_fltr;
	Integer32 adj;
	int s;

	/*
	 * calc 'slave_to_master_delay', removing the correction field
	 * added by transparent clocks in the path.
	 */
	sub_TimeInternal(s_to_m_dly, &ppi->t4,	&ppi->t3);
	sub_TimeInternal(s_to_m_dly, s_to_m_dly, &ppi->cField);
	pp_diag(ppi, servo, 3, "correction field 2: %s\n",
		fmt_TI(&ppi->cField));

	pp_diag(ppi, servo, 2, "T1: %s\n", fmt_TI(&ppi->t1));
	pp_diag(ppi, servo, 2, "T2: %s\n", fmt_TI(&ppi->t2));
	pp_diag(ppi, servo, 2, "T3: %s\n", fmt_TI(&ppi->t3));
	pp_diag(ppi, servo, 2, "T4: %s\n", fmt_TI(&ppi->t4));
	pp_diag(ppi, servo, 1, "Master to slave: %s\n", fmt_TI(m_to_s_dly));
	pp_diag(ppi, servo, 1, "Slave to master: %s\n", fmt_TI(s_to_m_dly));

	/* Calc mean path delay, used later to calc "offset from master" */
	add_TimeInternal(owd, &SRV(ppi)->m_to_s_dly, &SRV(ppi)->s_to_m_dly);
	div2_TimeInternal(owd);
	pp_diag(ppi, servo, 1, "One-way delay: %s\n", fmt_TI(owd));

	if(pp_servo_bad_event(ppi))
		return;

	if (owd_fltr->s_exp < 1) {
		/* First time, keep what we have */
		owd_fltr->y = owd->nanoseconds;
	}
	/* avoid overflowing filter */
	s = OPTS(ppi)->s;
	while (abs(owd_fltr->y) >> (31 - s))
		--s;
	if (owd_fltr->s_exp > 1 << s)
		owd_fltr->s_exp = 1 << s;
	/* crank down filter cutoff by increasing 's_exp' */
	if (owd_fltr->s_exp < 1 << s)
		++owd_fltr->s_exp;

	/*
	 * It may happen that owd appears as negative. This happens when
	 * the slave clock is running fast to recover a late time: the
	 * (t3 - t2) measured in the slave appears longer than the (t4 - t1)
	 * measured in the master.  Ignore such values, by keeping the
	 * current average instead.
	 */
	if (owd->nanoseconds < 0)
		owd->nanoseconds = owd_fltr->y;
	if (owd->nanoseconds < 0)
		owd->nanoseconds = 0;

	/*
	 * It may happen that owd appears to be very big. This happens
	 * when we have software timestamps and there is overhead
	 * involved -- or when the slave clock is running slow.  In
	 * this case use a value just slightly bigger than the current
	 * average (so if it really got longer, we will adapt).  This
	 * kills most outliers on loaded networks.
	 */
	if (owd->nanoseconds > 3 * owd_fltr->y) {
		pp_diag(ppi, servo, 1, "Trim too-long owd: %i\n",
			owd->nanoseconds);
		/* add fltr->s_exp to ensure we are not trapped into 0 */
		owd->nanoseconds = owd_fltr->y * 2 + owd_fltr->s_exp + 1;
	}
	/* filter 'oneWayDelay' (running average) */
	owd_fltr->y = (owd_fltr->y * (owd_fltr->s_exp - 1) + owd->nanoseconds)
		/ owd_fltr->s_exp;
	owd->nanoseconds = owd_fltr->y;

	pp_diag(ppi, servo, 1, "After avg(%i), one-way delay: %i\n",
		(int)owd_fltr->s_exp, owd->nanoseconds);

	/* update 'offsetFromMaster', (End to End mode) */
	sub_TimeInternal(ofm, m_to_s_dly, owd);
	pp_diag(ppi, servo, 2, "Offset from master:     %s\n", fmt_TI(ofm));

	if (OPTS(ppi)->max_rst) { /* If max_rst is 0 then it's OFF */
		if (ofm->seconds) {
			pp_diag(ppi, servo, 1, "servo aborted, offset greater "
				"than 1 second\n");
			return; /* not good */
		}

		if (ofm->nanoseconds > OPTS(ppi)->max_rst) {
			pp_diag(ppi, servo, 1, "servo aborted, offset greater "
				"than configured maximum %d\n",
				OPTS(ppi)->max_rst);
			return; /* not good */
		}
	}

	if (ofm->seconds) {
		TimeInternal time_tmp;

		/* if secs, reset clock or set freq adjustment to max */
		if (!OPTS(ppi)->no_adjust) {
			if (!OPTS(ppi)->no_rst_clk) {
				/* FIXME: use adjust instead of set? */
				ppi->t_ops->get(ppi, &time_tmp);
				sub_TimeInternal(&time_tmp, &time_tmp, ofm);
				ppi->t_ops->set(ppi, &time_tmp);
				pp_servo_init(ppi);
			} else {
				adj = ofm->nanoseconds > 0
					? PP_ADJ_FREQ_MAX : -PP_ADJ_FREQ_MAX;

				if (ppi->t_ops->adjust_freq)
					ppi->t_ops->adjust_freq(ppi, -adj);
				else
					ppi->t_ops->adjust_offset(ppi, -adj);
			}
		}
		return; /* ok */
	}

	/*
	 * Filter the ofm using the same running averags as we used for owd
	 */
	if (ofm_fltr->s_exp < 1) {
		/* First time, keep what we have */
		ofm_fltr->y = ofm->nanoseconds;
		ofm_fltr->m = abs(ofm->nanoseconds);
	}

	/* avoid overflowing filter */
	s = OPTS(ppi)->s;
	while (abs(ofm_fltr->y) >> (31 - s))
		--s;
	if (ofm_fltr->s_exp > 1 << s)
		ofm_fltr->s_exp = 1 << s;
	/* crank down filter cutoff by increasing 's_exp' */
	if (ofm_fltr->s_exp < 1 << s)
		++ofm_fltr->s_exp;

	/*
	 * Identify and discard outliers.
	 *
	 * Even though in a synced state it's good to average the OFM
	 * value, we must be careful when we are not yet synced: in
	 * that case we should not average at all and accept the
	 * outliers. avoiding the risk to drop good events in a
	 * changing-delay environment.
	 */
	if (abs(SRV(ppi)->obs_drift) >= PP_ADJ_FREQ_MAX) {
		/* do not average at all */
		ofm_fltr->s_exp = 1;
		ofm_fltr->y = ofm->nanoseconds;
		ofm_fltr->m = abs(ofm->nanoseconds);
	} else {
		/*
		 * Outliers are usually a few milliseconds off when we
		 * are synced at near microsecond magnitude. Knowing
		 * that high precision implementations (hw stamps)
		 * have no outliers, we can trim supposed-outliers to
		 * values that change the final averaged OFM by one magnitude
		 * of our past history. Thus, after we converged, we
		 * accept smaller changes, but doubling is allowed
		 */
		int delta = ofm->nanoseconds - ofm_fltr->y;
		int maxd = ofm_fltr->m * ofm_fltr->s_exp;

		if (abs(delta) > maxd) {
			if (delta > 0)
				ofm->nanoseconds  = ofm_fltr->y + maxd;
			else
				ofm->nanoseconds  = ofm_fltr->y - maxd;
			pp_diag(ppi, servo, 1, "Trimmed delta %i to %i\n",
				delta, maxd);
			pp_diag(ppi, servo, 1, "Use ofm = %9i\n",
				ofm->nanoseconds);
		}
	}
	/* filter 'offsetFromMaster' (running average) and its magnitude */
	ofm_fltr->y = (ofm_fltr->y * (ofm_fltr->s_exp - 1) + ofm->nanoseconds)
		/ ofm_fltr->s_exp;
	ofm_fltr->m = (ofm_fltr->m * (ofm_fltr->s_exp - 1) + abs(ofm_fltr->y))
		/ ofm_fltr->s_exp + 1 /* add 1 to avoid near-0 issues */;

	/* but if we changed sign, start averaging anew to limit oscillation */
	if (ofm->nanoseconds * ofm_fltr->y < 0)
		ofm_fltr->s_exp = 1;

	ofm->nanoseconds = ofm_fltr->y;

	pp_diag(ppi, servo, 1, "After avg(%i), mag %9i and ofm: %9i \n",
		(int)ofm_fltr->s_exp, ofm_fltr->m, ofm->nanoseconds);

	/*
	 * What follows is the PI controller: it has a problem, in that
	 * the same controller is used for offset and frequency adjustments.
	 * Since the controller is based on deltas, if the host uses
	 * adjust_freq() it has a stable offsey by design: the offset
	 * that keeps the controller asking for the proper frequency (if
	 * the offset were zero the controller would ask for 0 frequency
	 * adjustment, thus unsyncing the clocks in most cases. 
	 */

	/* the accumulator for the I component */
	SRV(ppi)->obs_drift += ofm->nanoseconds / OPTS(ppi)->ai;

	/* clamp the accumulator to PP_ADJ_FREQ_MAX for sanity */
	if (SRV(ppi)->obs_drift > PP_ADJ_FREQ_MAX)
		SRV(ppi)->obs_drift = PP_ADJ_FREQ_MAX;
	else if (SRV(ppi)->obs_drift < -PP_ADJ_FREQ_MAX)
		SRV(ppi)->obs_drift = -PP_ADJ_FREQ_MAX;

	adj = ofm->nanoseconds / OPTS(ppi)->ap +
		SRV(ppi)->obs_drift;

	/* apply controller output as a clock tick rate adjustment, if
	 * provided by arch, or as a raw offset otherwise */
	if (!OPTS(ppi)->no_adjust) {
		if (ppi->t_ops->adjust_freq)
			ppi->t_ops->adjust_freq(ppi, -adj);
		else
			ppi->t_ops->adjust_offset(ppi, -adj);
	}

	pp_diag(ppi, servo, 2, "One-way delay averaged: %s\n", fmt_TI(owd));
	pp_diag(ppi, servo, 2, "Offset from m averaged: %s\n", fmt_TI(ofm));
	pp_diag(ppi, servo, 2, "Observed drift: %9i\n",
		(int)SRV(ppi)->obs_drift);
}
