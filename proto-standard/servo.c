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
	clear_TimeInternal(&SRV(ppi)->m_to_s_dly);
	clear_TimeInternal(&SRV(ppi)->s_to_m_dly);
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


/*
 * Actual body of pp_servo_got_sync: the outer function prints logs
 * so we can return in different places.
 * Called by slave and uncalib when we have t1 and t2
 */
static int __pp_servo_got_sync(struct pp_instance *ppi)
{
	TimeInternal *correction_field = &ppi->cField;
	TimeInternal m_to_s_dly;
	TimeInternal time_tmp;
	struct pp_ofm_fltr *ofm_fltr = &SRV(ppi)->ofm_fltr;
	Integer32 adj;

	/* calc 'master_to_slave_delay' */
	sub_TimeInternal(&m_to_s_dly, &ppi->t2, &ppi->t1);

	if (OPTS(ppi)->max_dly) { /* If maxDelay is 0 then it's OFF */
		if (m_to_s_dly.seconds) {
			pp_diag(ppi, servo, 1, "%s aborted, delay greater "
				"than 1 second\n", __func__);
			return 0; /* not good */
		}
		if (m_to_s_dly.nanoseconds > OPTS(ppi)->max_dly) {
			pp_diag(ppi, servo, 1, "%s aborted, delay %d greater "
			     "than administratively set maximum %d\n",
			     __func__,
			     (int)m_to_s_dly.nanoseconds,
			     (int)OPTS(ppi)->max_dly);
			return 0; /* not good */
		}
	}

	SRV(ppi)->m_to_s_dly = m_to_s_dly;

	sub_TimeInternal(&SRV(ppi)->delay_ms, &ppi->t2, &ppi->t1);

	/* Take care about correctionField */
	sub_TimeInternal(&SRV(ppi)->m_to_s_dly,
		&SRV(ppi)->m_to_s_dly, correction_field);

	/* update 'offsetFromMaster', (End to End mode) */
	sub_TimeInternal(&DSCUR(ppi)->offsetFromMaster,
			&SRV(ppi)->m_to_s_dly,
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
			return 0; /* not good */
		}

		if ((DSCUR(ppi)->offsetFromMaster.nanoseconds) >
		    OPTS(ppi)->max_rst) {
			pp_diag(ppi, servo, 1, "%s aborted, offset %d greater "
			     "than administratively set maximum %d\n",
			     __func__,
			     (int)DSCUR(ppi)->offsetFromMaster.nanoseconds,
			     (int)OPTS(ppi)->max_rst);
			return 0; /* not good */
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
		return 1; /* ok */
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
	return 1; /* ok */
}

/* Called by slave and uncalib when we have t1 and t2 */
void pp_servo_got_sync(struct pp_instance *ppi)
{
	char s[24];

	SRV(ppi)->t1_t2_valid = __pp_servo_got_sync(ppi);
	if (!SRV(ppi)->t1_t2_valid) {
		/* error: message already reported */
		return;
	}

	/* Ok: print data */
	format_TimeInternal(s, &SRV(ppi)->m_to_s_dly);
	pp_diag(ppi, servo, 2, "Raw offset from master: %s\n", s);
	format_TimeInternal(s, &DSCUR(ppi)->meanPathDelay);
	pp_diag(ppi, servo, 2, "One-way delay averaged: %s\n", s);
	format_TimeInternal(s, &DSCUR(ppi)->offsetFromMaster);
	pp_diag(ppi, servo, 2, "Offset from master:     %s\n", s);
	pp_diag(ppi, servo, 2, "Observed drift: %9i\n",
		(int)SRV(ppi)->obs_drift);
}


/* called by slave states when delay_resp is received (all t1..t4 are valid) */
static void pp_update_delay(struct pp_instance *ppi,
			    TimeInternal *correction_field)
{
	TimeInternal s_to_m_dly;
	TimeInternal *mpd = &DSCUR(ppi)->meanPathDelay;
	struct pp_owd_fltr *owd_fltr = &SRV(ppi)->owd_fltr;
	int s;

	if (!SRV(ppi)->t1_t2_valid)
		return;

	/* calc 'slave to master' delay */
	sub_TimeInternal(&s_to_m_dly, &ppi->t4,	&ppi->t3);

	if (OPTS(ppi)->max_dly) { /* If max_delay is 0 then it's OFF */
		if (s_to_m_dly.seconds) {
			pp_diag(ppi, servo, 1, "%s aborted, delay "
				"greater than 1 second\n", __func__);
			return;
		}

		if (s_to_m_dly.nanoseconds > OPTS(ppi)->max_dly)
			pp_diag(ppi, servo, 1, "%s aborted, delay %d greater "
				   "than administratively set maximum %d\n",
				   __func__,
				   (int)s_to_m_dly.nanoseconds,
				   (int)OPTS(ppi)->max_dly);
		if (s_to_m_dly.nanoseconds > OPTS(ppi)->max_dly)
			return;
	}

	/* calc 'slave to_master' delay (master to slave delay is
	 * already computed in pp_update_offset)
	 */
	sub_TimeInternal(&SRV(ppi)->delay_sm, &ppi->t4,	&ppi->t3);

	/* update 'one_way_delay' */
	add_TimeInternal(mpd, &SRV(ppi)->delay_sm, &SRV(ppi)->delay_ms);

	/* Subtract correction_field */
	sub_TimeInternal(mpd, mpd, correction_field);

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
}

void pp_servo_got_resp(struct pp_instance *ppi)
{
	pp_update_delay(ppi, &ppi->cField);
}
