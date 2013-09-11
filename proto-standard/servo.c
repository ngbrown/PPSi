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

/* internal helper */
static void format_TimeInternal(char *s, TimeInternal *t)
{
	pp_sprintf(s, "%s%d.%09d",
		(t->seconds < 0 || (t->seconds == 0 && t->nanoseconds < 0))
		   ? "-" : " ",
		   (int)abs(t->seconds), (int)abs(t->nanoseconds));
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
}

/* called by slave states when delay_resp is received (all t1..t4 are valid) */
void pp_servo_got_resp(struct pp_instance *ppi)
{
	TimeInternal *m_to_s_dly = &SRV(ppi)->m_to_s_dly;
	TimeInternal *s_to_m_dly = &SRV(ppi)->s_to_m_dly;
	TimeInternal time_tmp;
	TimeInternal *mpd = &DSCUR(ppi)->meanPathDelay;
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

	/* update 'offsetFromMaster', (End to End mode) */
	sub_TimeInternal(&DSCUR(ppi)->offsetFromMaster,
			m_to_s_dly,
			&DSCUR(ppi)->meanPathDelay);

	if (DSCUR(ppi)->offsetFromMaster.seconds) {
		/* cannot filter with secs, clear filter */
		ofm_fltr->nsec_prev = 0;
		goto adjust;
	}
	/* filter 'offsetFromMaster' */
	ofm_fltr->y = DSCUR(ppi)->offsetFromMaster.nanoseconds / 2 +
		ofm_fltr->nsec_prev / 2;
	ofm_fltr->nsec_prev = DSCUR(ppi)->offsetFromMaster.nanoseconds;
	DSCUR(ppi)->offsetFromMaster.nanoseconds = ofm_fltr->y;

	if (OPTS(ppi)->max_rst) { /* If max_rst is 0 then it's OFF */
		if (DSCUR(ppi)->offsetFromMaster.seconds) {
			pp_diag(ppi, servo, 1, "%s aborted, offset greater "
				"than 1 second\n", __func__);
			return; /* not good */
		}

		if ((DSCUR(ppi)->offsetFromMaster.nanoseconds) >
		    OPTS(ppi)->max_rst) {
			pp_diag(ppi, servo, 1, "%s aborted, offset %d greater "
			     "than administratively set maximum %d\n",
			     __func__,
			     (int)DSCUR(ppi)->offsetFromMaster.nanoseconds,
			     (int)OPTS(ppi)->max_rst);
			return; /* not good */
		}
	}

adjust:
	if (DSCUR(ppi)->offsetFromMaster.seconds) {
		/* if secs, reset clock or set freq adjustment to max */
		if (!OPTS(ppi)->no_adjust) {
			if (!OPTS(ppi)->no_rst_clk) {
				/* FIXME: use adjust instead of set? */
				ppi->t_ops->get(ppi, &time_tmp);
				sub_TimeInternal(&time_tmp, &time_tmp,
					&DSCUR(ppi)->offsetFromMaster);
				ppi->t_ops->set(ppi, &time_tmp);
				pp_servo_init(ppi);
			} else {
				adj = DSCUR(ppi)->offsetFromMaster.nanoseconds
					> 0 ? PP_ADJ_FREQ_MAX:-PP_ADJ_FREQ_MAX;

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
	SRV(ppi)->obs_drift +=
		DSCUR(ppi)->offsetFromMaster.nanoseconds /
		OPTS(ppi)->ai;

	/* clamp the accumulator to PP_ADJ_FREQ_MAX for sanity */
	if (SRV(ppi)->obs_drift > PP_ADJ_FREQ_MAX)
		SRV(ppi)->obs_drift = PP_ADJ_FREQ_MAX;
	else if (SRV(ppi)->obs_drift < -PP_ADJ_FREQ_MAX)
		SRV(ppi)->obs_drift = -PP_ADJ_FREQ_MAX;

	adj = DSCUR(ppi)->offsetFromMaster.nanoseconds / OPTS(ppi)->ap +
		SRV(ppi)->obs_drift;

	/* apply controller output as a clock tick rate adjustment, if
	 * provided by arch, or as a raw offset otherwise */
	if (!OPTS(ppi)->no_adjust) {
		if (ppi->t_ops->adjust_freq)
			ppi->t_ops->adjust_freq(ppi, -adj);
		else
			ppi->t_ops->adjust_offset(ppi, -adj);
	}

	/* update 'one_way_delay' */
	add_TimeInternal(mpd, &SRV(ppi)->m_to_s_dly, &SRV(ppi)->s_to_m_dly);

	/* Compute one-way delay */
	div2_TimeInternal(mpd);

	if (mpd->seconds) {
		/* cannot filter with secs, clear filter */
		owd_fltr->s_exp = 0;
		owd_fltr->nsec_prev = 0;
		return;
	}

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
	mpd->nanoseconds = (mpd->nanoseconds + owd_fltr->nsec_prev) / 2;
	owd_fltr->nsec_prev = mpd->nanoseconds;

	/* filter 'meanPathDelay' (running average) */
	owd_fltr->y = (owd_fltr->y * (owd_fltr->s_exp - 1) + mpd->nanoseconds)
		/ owd_fltr->s_exp;

	mpd->nanoseconds = owd_fltr->y;

	pp_diag(ppi, servo, 1, "delay filter %d, %d\n",
		(int)owd_fltr->y, (int)owd_fltr->s_exp);

	/* Ok: print data if asked to, or don't even format_TimeInternal */
	if (pp_diag_allow(ppi, servo, 2)) {
		char s[24];

		format_TimeInternal(s, &SRV(ppi)->m_to_s_dly);
		pp_diag(ppi, servo, 2, "Raw offset from master: %s\n", s);
		format_TimeInternal(s, &DSCUR(ppi)->meanPathDelay);
		pp_diag(ppi, servo, 2, "One-way delay averaged: %s\n", s);
		format_TimeInternal(s, &DSCUR(ppi)->offsetFromMaster);
		pp_diag(ppi, servo, 2, "Offset from master:     %s\n", s);
		pp_diag(ppi, servo, 2, "Observed drift: %9i\n",
			(int)SRV(ppi)->obs_drift);
	}
}
