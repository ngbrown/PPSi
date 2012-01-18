/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <pptp/pptp.h>

void pp_init_clock(struct pp_instance *ppi)
{
	/* FIXME diag DBG("initClock\n"); */

	/* clear vars */
	set_TimeInternal(&SRV(ppi)->m_to_s_dly, 0, 0);
	set_TimeInternal(&SRV(ppi)->s_to_m_dly, 0, 0);
	SRV(ppi)->obs_drift = 0;	/* clears clock servo accumulator (the
					 * I term) */
	SRV(ppi)->owd_fltr.s_exp = 0;	/* clears one-way delay filter */

	/* level clock */
	if (!OPTS(ppi)->no_adjust)
		pp_adj_freq(0);
}

void pp_update_delay(struct pp_instance *ppi, TimeInternal *correction_field)
{
	TimeInternal s_to_m_dly;
	struct pp_owd_fltr *owd_fltr = &SRV(ppi)->owd_fltr;

	/* calc 'slave to master' delay */
	sub_TimeInternal(&s_to_m_dly, &ppi->delay_req_receive_time,
		&ppi->delay_req_send_time);

	if (OPTS(ppi)->max_dly) { /* If max_delay is 0 then it's OFF */
		if (s_to_m_dly.seconds) {
			/*FIXME diag
			 * INFO("updateDelay aborted, delay greater than 1"
			     " second.");
			*/
			return;
		}

		if (s_to_m_dly.nanoseconds > OPTS(ppi)->max_dly) {
			/*FIXME diag
			INFO("updateDelay aborted, delay %d greater than "
			     "administratively set maximum %d\n",
			     slave_to_master_delay.nanoseconds,
			     rtOpts->maxDelay);
			*/
			return;
		}
	}

	if (OPTS(ppi)->ofst_first_updated) {
		Integer16 s;

		/* calc 'slave to_master' delay (master to slave delay is
		 * already computed in pp_update_offset)
		 */
		sub_TimeInternal(&SRV(ppi)->delay_sm,
			&ppi->delay_req_receive_time,
			&ppi->delay_req_send_time);

		/* update 'one_way_delay' */
		add_TimeInternal(&DSCUR(ppi)->meanPathDelay,
				 &SRV(ppi)->delay_sm, &SRV(ppi)->delay_ms);

		/* Substract correction_field */
		sub_TimeInternal(&DSCUR(ppi)->meanPathDelay,
				 &DSCUR(ppi)->meanPathDelay, correction_field);

		/* Compute one-way delay */
		DSCUR(ppi)->meanPathDelay.seconds /= 2;
		DSCUR(ppi)->meanPathDelay.nanoseconds /= 2;


		if (DSCUR(ppi)->meanPathDelay.seconds) {
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

		/* filter 'meanPathDelay' */
		owd_fltr->y = (owd_fltr->s_exp - 1) *
			owd_fltr->y / owd_fltr->s_exp +
			(DSCUR(ppi)->meanPathDelay.nanoseconds / 2 +
			 owd_fltr->nsec_prev / 2) / owd_fltr->s_exp;

		owd_fltr->nsec_prev = DSCUR(ppi)->meanPathDelay.nanoseconds;
		DSCUR(ppi)->meanPathDelay.nanoseconds = owd_fltr->y;

		/*FIXME diag
		 * DBGV("delay filter %d, %d\n", owd_filt->y, owd_fltr->s_exp);
		 */
	}
}

void pp_update_offset(struct pp_instance *ppi, TimeInternal *send_time,
		      TimeInternal *recv_time,
		      TimeInternal *correctionField)
			/* FIXME: offset_from_master_filter: put it in ppi */
{
	/* TODO servo implement function*/
}

void pp_update_clock(struct pp_instance *ppi)
{
	/* TODO servo implement function*/
}

