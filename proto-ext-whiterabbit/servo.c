/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

void pp_init_clock(struct pp_instance *ppi)
{
	PP_PRINTF("SERVO init clock\n");

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
			PP_VPRINTF("pp_update_delay aborted, delay greater than 1"
			     " second\n");
			return;
		}

		if (s_to_m_dly.nanoseconds > OPTS(ppi)->max_dly) {
			PP_VPRINTF("pp_update_delay aborted, delay %d greater than "
			     "administratively set maximum %d\n",
			     s_to_m_dly.nanoseconds,
			     OPTS(ppi)->max_dly);
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
		div2_TimeInternal(&DSCUR(ppi)->meanPathDelay);


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

		PP_VPRINTF("delay filter %d, %d\n",
			   owd_fltr->y, owd_fltr->s_exp);
	}
}

void pp_update_peer_delay(struct pp_instance *ppi,
			  TimeInternal *correction_field, int two_step)
{
	Integer16 s;
	struct pp_owd_fltr *owd_fltr = &SRV(ppi)->owd_fltr;

	PP_VPRINTF("pp_update_peer_delay\n");

	if (two_step) {
		/* calc 'slave_to_master_delay' */
		sub_TimeInternal(&SRV(ppi)->pdelay_ms,
			&ppi->pdelay_resp_receive_time,
			&ppi->pdelay_resp_send_time);
		sub_TimeInternal(&SRV(ppi)->pdelay_sm,
			&ppi->pdelay_req_receive_time,
			&ppi->pdelay_req_send_time);

		/* update 'one_way_delay' */
		add_TimeInternal(&DSPOR(ppi)->peerMeanPathDelay,
			&SRV(ppi)->pdelay_ms,
			&SRV(ppi)->pdelay_sm);

		/* Substract correctionField */
		sub_TimeInternal(&DSPOR(ppi)->peerMeanPathDelay,
			&DSPOR(ppi)->peerMeanPathDelay, correction_field);

		/* Compute one-way delay */
		div2_TimeInternal(&DSPOR(ppi)->peerMeanPathDelay);
	} else {
		/* One step clock */

		sub_TimeInternal(&DSPOR(ppi)->peerMeanPathDelay,
			&ppi->pdelay_resp_receive_time,
			&ppi->pdelay_req_send_time);

		/* Substract correctionField */
		sub_TimeInternal(&DSPOR(ppi)->peerMeanPathDelay,
			&DSPOR(ppi)->peerMeanPathDelay, correction_field);

		/* Compute one-way delay */
		div2_TimeInternal(&DSPOR(ppi)->peerMeanPathDelay);
	}

	if (DSPOR(ppi)->peerMeanPathDelay.seconds) {
		/* cannot filter with secs, clear filter */
		owd_fltr->s_exp = owd_fltr->nsec_prev = 0;
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
		(DSPOR(ppi)->peerMeanPathDelay.nanoseconds / 2 +
		 owd_fltr->nsec_prev / 2) / owd_fltr->s_exp;

	owd_fltr->nsec_prev = DSPOR(ppi)->peerMeanPathDelay.nanoseconds;
	DSPOR(ppi)->peerMeanPathDelay.nanoseconds = owd_fltr->y;

	PP_VPRINTF("delay filter %d, %d\n", owd_fltr->y, owd_fltr->s_exp);
}

void pp_update_offset(struct pp_instance *ppi, TimeInternal *send_time,
		      TimeInternal *recv_time, TimeInternal *correction_field)
{
	TimeInternal m_to_s_dly;
	struct pp_ofm_fltr *ofm_fltr = &SRV(ppi)->ofm_fltr;

	PP_VPRINTF("pp_update_offset\n");

	/* calc 'master_to_slave_delay' */
	sub_TimeInternal(&m_to_s_dly, recv_time, send_time);

	if (OPTS(ppi)->max_dly) { /* If maxDelay is 0 then it's OFF */
		if (m_to_s_dly.seconds) {
			PP_PRINTF("pp_update_offset aborted, delay greater "
			     "than 1 second\n");
			return;
		}

		if (m_to_s_dly.nanoseconds > OPTS(ppi)->max_dly) {
			PP_PRINTF("updateDelay aborted, delay %d greater than "
				  "administratively set maximum %d\n",
			     m_to_s_dly.nanoseconds,
			     OPTS(ppi)->max_dly);
			return;
		}
	}

	SRV(ppi)->m_to_s_dly = m_to_s_dly;

	sub_TimeInternal(&SRV(ppi)->delay_ms, recv_time, send_time);
	/* Used just for End to End mode. */

	/* Take care about correctionField */
	sub_TimeInternal(&SRV(ppi)->m_to_s_dly,
		&SRV(ppi)->m_to_s_dly, correction_field);

	/* update 'offsetFromMaster' */
	if (!OPTS(ppi)->e2e_mode) {
		sub_TimeInternal(&DSCUR(ppi)->offsetFromMaster,
			&SRV(ppi)->m_to_s_dly,
			&DSPOR(ppi)->peerMeanPathDelay);
	} else {
		/* (End to End mode) */
		sub_TimeInternal(&DSCUR(ppi)->offsetFromMaster,
			&SRV(ppi)->m_to_s_dly,
			&DSCUR(ppi)->meanPathDelay);
	}

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

	PP_VPRINTF("offset filter %d\n", ofm_fltr->y);

	/* Offset must have been computed at least one time before
	 * computing end to end delay */
	if (!OPTS(ppi)->ofst_first_updated)
		OPTS(ppi)->ofst_first_updated = 1;
}

void pp_update_clock(struct pp_instance *ppi)
{
	Integer32 adj;
	TimeInternal time_tmp;
	uint32_t tstamp_diff;

	PP_VPRINTF("pp_update_clock\n");

	if (OPTS(ppi)->max_rst) { /* If max_rst is 0 then it's OFF */
		if (DSCUR(ppi)->offsetFromMaster.seconds) {
			PP_PRINTF("pp_update_clock aborted, offset greater "
				   "than 1 second\n");
			goto display;
		}

		if (DSCUR(ppi)->offsetFromMaster.nanoseconds >
			OPTS(ppi)->max_rst) {
			PP_VPRINTF("pp_update_clock aborted, offset %d greater than "
			     "administratively set maximum %d\n",
			     DSCUR(ppi)->offsetFromMaster.nanoseconds,
			     OPTS(ppi)->max_rst);
			goto display;
		}
	}

	if (DSCUR(ppi)->offsetFromMaster.seconds) {
		/* if secs, reset clock or set freq adjustment to max */
		if (!OPTS(ppi)->no_adjust) {
			if (!OPTS(ppi)->no_rst_clk) {
				pp_get_tstamp(&time_tmp);
				sub_TimeInternal(&time_tmp, &time_tmp,
					&DSCUR(ppi)->offsetFromMaster);
				tstamp_diff = pp_set_tstamp(&time_tmp);
				pp_timer_adjust_all(ppi, tstamp_diff);
				pp_init_clock(ppi);
			} else {
				adj = DSCUR(ppi)->offsetFromMaster.nanoseconds
					> 0 ? PP_ADJ_FREQ_MAX:-PP_ADJ_FREQ_MAX;
				pp_adj_freq(-adj);
			}
		}
	} else {
		static int dc = 0;
		/* the PI controller */

		/* no negative or zero attenuation */
		if (OPTS(ppi)->ap < 1)
			OPTS(ppi)->ap = 1;
		if (OPTS(ppi)->ai < 1)
			OPTS(ppi)->ai = 1;

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
			pp_adj_freq(-adj);

		dc++;
		if (dc % 2 == 0) { /* Prints statistics every 8s */
				PP_PRINTF("ofst %d, raw ofst %d, mean-dly %d, adj %d\n",
					DSCUR(ppi)->offsetFromMaster.nanoseconds,
					SRV(ppi)->m_to_s_dly.nanoseconds,
					DSCUR(ppi)->meanPathDelay.nanoseconds,
					adj);

		}
	}

display:
	/* FIXME diag
	if (rtOpts->displayStats)
		displayStats(rtOpts, ptpClock);
	*/


	PP_VPRINTF("\n--Offset Correction-- \n");
	PP_VPRINTF("Raw offset from master:  %10ds %11dns\n",
		SRV(ppi)->m_to_s_dly.seconds,
		SRV(ppi)->m_to_s_dly.nanoseconds);

	PP_VPRINTF("\n--Offset and Delay filtered-- \n");

	if (!OPTS(ppi)->e2e_mode) {
		PP_VPRINTF("one-way delay averaged (P2P):  %10ds %11dns\n",
		    DSPOR(ppi)->peerMeanPathDelay.seconds,
		    DSPOR(ppi)->peerMeanPathDelay.nanoseconds);
	} else {
		PP_VPRINTF("one-way delay averaged (E2E):  %10ds %11dns\n",
		    DSCUR(ppi)->meanPathDelay.seconds,
		    DSCUR(ppi)->meanPathDelay.nanoseconds);
	}

	PP_VPRINTF("offset from master:      %10ds %11dns\n",
	    DSCUR(ppi)->offsetFromMaster.seconds,
	    DSCUR(ppi)->offsetFromMaster.nanoseconds);
	PP_VPRINTF("observed drift:          %10d\n", SRV(ppi)->obs_drift);

}