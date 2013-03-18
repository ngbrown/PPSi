/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>

void pp_init_clock(struct pp_instance *ppi)
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

/* called by slave states */
void pp_update_delay(struct pp_instance *ppi, TimeInternal *correction_field)
{
	TimeInternal s_to_m_dly;
	TimeInternal *mpd = &DSCUR(ppi)->meanPathDelay;
	struct pp_owd_fltr *owd_fltr = &SRV(ppi)->owd_fltr;
	int s;

	/* calc 'slave to master' delay */
	sub_TimeInternal(&s_to_m_dly, &ppi->t4,	&ppi->t3);

	if (OPTS(ppi)->max_dly) { /* If max_delay is 0 then it's OFF */
		pp_diag(ppi, servo, 1, "%s aborted, delay "
				   "greater than 1 second\n", __func__);
		if (s_to_m_dly.seconds)
			return;

		if (s_to_m_dly.nanoseconds > OPTS(ppi)->max_dly)
			pp_diag(ppi, servo, 1, "%s aborted, delay %d greater "
				   "than administratively set maximum %d\n",
				   __func__,
				   s_to_m_dly.nanoseconds,
				   OPTS(ppi)->max_dly);
		if (s_to_m_dly.nanoseconds > OPTS(ppi)->max_dly)
			return;
	}

	if (!OPTS(ppi)->ofst_first_updated)
		return;

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
		owd_fltr->y, owd_fltr->s_exp);
}

/*
 * Called by slave and uncalib.
 * Please note that it only uses t1 and t2, so I think it needs review - ARub
 */
void pp_update_offset(struct pp_instance *ppi, TimeInternal *correction_field)
{
	TimeInternal m_to_s_dly;
	struct pp_ofm_fltr *ofm_fltr = &SRV(ppi)->ofm_fltr;

	/* calc 'master_to_slave_delay' */
	sub_TimeInternal(&m_to_s_dly, &ppi->t2, &ppi->t1);

	if (OPTS(ppi)->max_dly) { /* If maxDelay is 0 then it's OFF */
		if (m_to_s_dly.seconds) {
			pp_diag(ppi, servo, 1, "%s aborted, delay greater "
				"than 1 second\n", __func__);
			return;
		}
		if (m_to_s_dly.nanoseconds > OPTS(ppi)->max_dly) {
			pp_diag(ppi, servo, 1, "%s aborted, delay %d greater "
			     "than administratively set maximum %d\n",
			     __func__,
			     m_to_s_dly.nanoseconds,
			     OPTS(ppi)->max_dly);
			return;
		}
	}

	SRV(ppi)->m_to_s_dly = m_to_s_dly;

	sub_TimeInternal(&SRV(ppi)->delay_ms, &ppi->t2, &ppi->t1);
	/* Used just for End to End mode. */

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
		return;
	}
	/* filter 'offsetFromMaster' */
	ofm_fltr->y = DSCUR(ppi)->offsetFromMaster.nanoseconds / 2 +
		ofm_fltr->nsec_prev / 2;
	ofm_fltr->nsec_prev = DSCUR(ppi)->offsetFromMaster.nanoseconds;
	DSCUR(ppi)->offsetFromMaster.nanoseconds = ofm_fltr->y;

	/* Offset must have been computed at least one time before
	 * computing end to end delay */
	if (!OPTS(ppi)->ofst_first_updated)
		OPTS(ppi)->ofst_first_updated = 1;
}

/* This internal code is used to avoid "goto display" and always have diags */
static void __pp_update_clock(struct pp_instance *ppi)
{
	Integer32 adj;
	TimeInternal time_tmp;

	if (OPTS(ppi)->max_rst) { /* If max_rst is 0 then it's OFF */
		if (DSCUR(ppi)->offsetFromMaster.seconds) {
			pp_diag(ppi, servo, 1, "%s aborted, offset greater "
				"than 1 second\n", __func__);
			return;
		}

		if ((DSCUR(ppi)->offsetFromMaster.nanoseconds) >
		    OPTS(ppi)->max_rst) {
			pp_diag(ppi, servo, 1, "%s aborted, offset %d greater "
			     "than administratively set maximum %d\n",
			     __func__,
			     DSCUR(ppi)->offsetFromMaster.nanoseconds,
			     OPTS(ppi)->max_rst);
			return;
		}
	}

	if (DSCUR(ppi)->offsetFromMaster.seconds) {
		/* if secs, reset clock or set freq adjustment to max */
		if (!OPTS(ppi)->no_adjust) {
			if (!OPTS(ppi)->no_rst_clk) {
				/* FIXME: use adjust instead of set? */
				ppi->t_ops->get(ppi, &time_tmp);
				sub_TimeInternal(&time_tmp, &time_tmp,
					&DSCUR(ppi)->offsetFromMaster);
				ppi->t_ops->set(ppi, &time_tmp);
				pp_init_clock(ppi);
			} else {
				adj = DSCUR(ppi)->offsetFromMaster.nanoseconds
					> 0 ? PP_ADJ_NS_MAX:-PP_ADJ_NS_MAX;
				ppi->t_ops->adjust(ppi, -adj, 0);
			}
		}
		return;
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

	/* apply controller output as a clock tick rate adjustment */
	if (!OPTS(ppi)->no_adjust)
		ppi->t_ops->adjust(ppi, 0, -adj);
}

/* called only *exactly* after calling pp_update_offset above */
void pp_update_clock(struct pp_instance *ppi)
{
	__pp_update_clock(ppi);

	pp_diag(ppi, servo, 2, "Raw offset from master: %9i.%09i\n",
		SRV(ppi)->m_to_s_dly.seconds,
		SRV(ppi)->m_to_s_dly.nanoseconds);
	pp_diag(ppi, servo, 2, "One-way delay averaged: %9i.%09i\n",
		DSCUR(ppi)->meanPathDelay.seconds,
		DSCUR(ppi)->meanPathDelay.nanoseconds);
	pp_diag(ppi, servo, 2, "Offset from master:     %9i.%09i\n",
		DSCUR(ppi)->offsetFromMaster.seconds,
		DSCUR(ppi)->offsetFromMaster.nanoseconds);
	pp_diag(ppi, servo, 2, "Observed drift: %9i\n", SRV(ppi)->obs_drift);
}
