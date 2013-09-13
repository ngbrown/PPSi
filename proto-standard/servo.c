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
	pp_diag(ppi, servo, 1, "Initializing\n");

	/* clear vars */
	SRV(ppi)->obs_drift = 0;	/* clears clock servo accumulator (the
					 * I term) */
	SRV(ppi)->owd_fltr.s_exp = 0;	/* clears one-way delay filter */

	/* level clock */
	if (!OPTS(ppi)->no_adjust)
		ppi->t_ops->adjust(ppi, 0, 0);
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

/* called by slave states when delay_resp is received (all t1..t4 are valid) */
void pp_servo_got_resp(struct pp_instance *ppi)
{
	TimeInternal *m_to_s_dly = &SRV(ppi)->m_to_s_dly;
	TimeInternal *s_to_m_dly = &SRV(ppi)->s_to_m_dly;
	TimeInternal *owd = &DSCUR(ppi)->oneWayDelay;
	TimeInternal *ofm = &DSCUR(ppi)->offsetFromMaster;
	struct pp_ofm_fltr *ofm_fltr = &SRV(ppi)->ofm_fltr;
	struct pp_owd_fltr *owd_fltr = &SRV(ppi)->owd_fltr;
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

	/* Check for too-big offsets, and then make the calculation */

	if (OPTS(ppi)->max_dly) { /* If maxDelay is 0 then it's OFF */
		if (m_to_s_dly->seconds) {
			pp_diag(ppi, servo, 1, "%s aborted, delay greater "
				"than 1 second\n", __func__);
			return;
		}
		if (m_to_s_dly->nanoseconds > OPTS(ppi)->max_dly) {
			pp_diag(ppi, servo, 1, "%s aborted, delay %d greater "
			     "than administratively set maximum %d\n",
			     __func__,
			     (int)m_to_s_dly->nanoseconds,
			     (int)OPTS(ppi)->max_dly);
			return;
		}

		if (s_to_m_dly->seconds) {
			pp_diag(ppi, servo, 1, "%s aborted, delay "
				"greater than 1 second\n", __func__);
			return;
		}

		if (s_to_m_dly->nanoseconds > OPTS(ppi)->max_dly)
			pp_diag(ppi, servo, 1, "%s aborted, delay %d greater "
				   "than administratively set maximum %d\n",
				   __func__,
				   (int)s_to_m_dly->nanoseconds,
				   (int)OPTS(ppi)->max_dly);
		if (s_to_m_dly->nanoseconds > OPTS(ppi)->max_dly)
			return;
	}

	/* Calc mean path delay, used later to calc "offset from master" */
	add_TimeInternal(owd, &SRV(ppi)->m_to_s_dly, &SRV(ppi)->s_to_m_dly);
	div2_TimeInternal(owd);
	pp_diag(ppi, servo, 1, "One-way delay: %s\n", fmt_TI(owd));

	if (owd->seconds) {
		/* cannot filter with secs, clear filter */
		owd_fltr->s_exp = 0;
		owd_fltr->nsec_prev = 0;
	} else {
		/* avoid overflowing filter */
		s = OPTS(ppi)->s;
		while (abs(owd_fltr->y) >> (31 - s))
			--s;

		/* crank down filter cutoff by increasing 's_exp' */
		if (owd_fltr->s_exp < 1)
			owd_fltr->s_exp = 1;
		else if (owd_fltr->s_exp < 1 << s)
			++owd_fltr->s_exp;
		else if (owd_fltr->s_exp > 1 << s)
			owd_fltr->s_exp = 1 << s;

		/* Use the average between current value and previous one */
		owd->nanoseconds = (owd->nanoseconds + owd_fltr->nsec_prev) / 2;
		owd_fltr->nsec_prev = owd->nanoseconds;

		/* filter 'oneWayDelay' (running average) */
		owd_fltr->y = (owd_fltr->y * (owd_fltr->s_exp - 1)
			       + owd->nanoseconds)
			/ owd_fltr->s_exp;

		owd->nanoseconds = owd_fltr->y;

		pp_diag(ppi, servo, 1, "After avg(%i), one-way delay: %i\n",
			(int)owd_fltr->s_exp, owd->nanoseconds);
	}

	/* update 'offsetFromMaster', (End to End mode) */
	sub_TimeInternal(ofm, m_to_s_dly, owd);

	if (ofm->seconds) {
		/* cannot filter with secs, clear filter */
		ofm_fltr->nsec_prev = 0;
		goto adjust;
	}
	/* filter 'offsetFromMaster' */
	ofm_fltr->y = (ofm->nanoseconds + ofm_fltr->nsec_prev) / 2;
	ofm_fltr->nsec_prev = ofm->nanoseconds;
	ofm->nanoseconds = ofm_fltr->y;

	if (OPTS(ppi)->max_rst) { /* If max_rst is 0 then it's OFF */
		if (ofm->seconds) {
			pp_diag(ppi, servo, 1, "%s aborted, offset greater "
				"than 1 second\n", __func__);
			return; /* not good */
		}

		if (ofm->nanoseconds > OPTS(ppi)->max_rst) {
			pp_diag(ppi, servo, 1, "%s aborted, offset %d greater "
				"than administratively set maximum %d\n",
				__func__, ofm->nanoseconds,
				OPTS(ppi)->max_rst);
			return; /* not good */
		}
	}

adjust:
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

	/* the PI controller */

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
	pp_diag(ppi, servo, 2, "Offset from master:     %s\n", fmt_TI(ofm));
	pp_diag(ppi, servo, 2, "Observed drift: %9i\n",
		(int)SRV(ppi)->obs_drift);
}
