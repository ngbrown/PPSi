#include <arch/arch.h>
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "wr-api.h"

#define WR_SYNC_NSEC 1
#define WR_SYNC_TAI 2
#define WR_SYNC_PHASE 3
#define WR_TRACK_PHASE 4
#define WR_WAIT_SYNC_IDLE 5
#define WR_WAIT_OFFSET_STABLE 6

#define WR_SERVO_OFFSET_STABILITY_THRESHOLD 60 /* psec */

#define FIX_ALPHA_FRACBITS 40

const char *servo_state_str[] = {
	[WR_SYNC_NSEC] = "SYNC_NSEC",
	[WR_SYNC_TAI] = "SYNC_SEC",
	[WR_SYNC_PHASE] = "SYNC_PHASE",
	[WR_TRACK_PHASE] = "TRACK_PHASE",
	[WR_WAIT_SYNC_IDLE] = "SYNC_IDLE",
	[WR_WAIT_OFFSET_STABLE] = "OFFSET_STABLE",
};

int servo_state_valid = 0; /* FIXME: why? */
ptpdexp_sync_state_t cur_servo_state; /* FIXME: why? */

static int tracking_enabled = 1; /* FIXME: why? */

void wr_servo_enable_tracking(int enable)
{
	tracking_enabled = enable;
}

/* my own timestamp arithmetic functions */

static void dump_timestamp(char *what, TimeInternal ts)
{
	PP_PRINTF("%s = %d:%d:%d\n", what, (int32_t)ts.seconds, ts.nanoseconds, ts.phase);
}

static int64_t ts_to_picos(TimeInternal ts)
{
	return (int64_t) ts.seconds * (int64_t)1000000000000LL
		+ (int64_t) ts.nanoseconds * (int64_t)1000LL
		+ (int64_t) ts.phase;
}

static TimeInternal picos_to_ts(int64_t picos)
{
	int64_t phase = picos % 1000LL;
	picos = (picos - phase) / 1000LL;

	int64_t nsec = picos % 1000000000LL;
	picos = (picos-nsec)/1000000000LL;

	TimeInternal ts;
	ts.seconds = (int64_t) picos;
	ts.nanoseconds = (int32_t) nsec;
	ts.phase = (int32_t) phase;

	return ts;
}

static TimeInternal ts_add(TimeInternal a, TimeInternal b)
{
	TimeInternal c;

	c.seconds = 0;
	c.nanoseconds = 0;

	c.phase =a.phase + b.phase;

	while(c.phase >= 1000)
	{
		c.phase -= 1000;
		c.nanoseconds++;
	}

	c.nanoseconds += (a.nanoseconds + b.nanoseconds);

	while(c.nanoseconds >= 1000000000L)
	{
		c.nanoseconds -= 1000000000L;
		c.seconds++;
	}

	c.seconds += (a.seconds + b.seconds);

	return c;
}

static TimeInternal ts_sub(TimeInternal a, TimeInternal b)
{
	TimeInternal c;

	c.seconds = 0;
	c.nanoseconds = 0;

	c.phase = a.phase - b.phase;

	while(c.phase < 0)
	{
		c.phase+=1000;
		c.nanoseconds--;
	}

	c.nanoseconds += a.nanoseconds - b.nanoseconds;
	while(c.nanoseconds < 0)
	{
		c.nanoseconds += 1000000000L;
		c.seconds--;
	}

	c.seconds += a.seconds - b.seconds;

	return c;
}

/* "Hardwarizes" the timestamp - e.g. makes the nanosecond field a multiple
 * of 8ns cycles and puts the extra nanoseconds in the phase field */
static TimeInternal ts_hardwarize(TimeInternal ts, int clock_period_ps)
{
    int32_t q_threshold;

    q_threshold = (clock_period_ps + 999) / 1000;

	if(ts.nanoseconds > 0)
	{
		int32_t extra_nsec = ts.nanoseconds % q_threshold;

		if(extra_nsec)
		{
			ts.nanoseconds -=extra_nsec;
			ts.phase += extra_nsec * 1000;
		}
	}

	if(ts.nanoseconds < 0) {
		ts.nanoseconds += 1000000000LL;
		ts.seconds--;
	}

    if(ts.seconds == -1 && ts.nanoseconds > 0)
    {
        ts.seconds++;
        ts.nanoseconds -= 1000000000LL;
    }

    if(ts.nanoseconds < 0 && ts.nanoseconds >= (-q_threshold) && ts.seconds == 0)
    {
        ts.nanoseconds += q_threshold;
        ts.phase -= q_threshold * 1000;
    }

	return ts;
}

/* end my own timestamp arithmetic functions */

static int got_sync = 0;

void wr_servo_reset()
{
//    shw_pps_gen_enable_output(0); /* fixme: unportable */
	cur_servo_state.valid = 0;
	servo_state_valid = 0;
//    ptpd_netif_enable_timing_output(0);

}

int wr_servo_init(struct pp_instance *ppi)
{
	struct wr_servo_state_t *s =
			&((struct wr_data_t *)ppi->ext_data)->servo_state;

	/* Determine the alpha coefficient */
	if (wr_read_calibration_data(ppi, 0, 0,
		&s->fiber_fix_alpha, &s->clock_period_ps) != WR_HW_CALIB_OK)
		return -1;

	wr_enable_timing_output(ppi, 0);

	/* FIXME useful?
	strncpy(s->if_name, clock->netPath.ifaceName, 16);
	*/

	s->state = WR_SYNC_TAI;
	s->cur_setpoint = 0;
	s->missed_iters = 0;
	
	s->delta_tx_m = ((((int32_t)WR_DSPOR(ppi)->otherNodeDeltaTx.scaledPicoseconds.lsb) >> 16) & 0xffff) | (((int32_t)WR_DSPOR(ppi)->otherNodeDeltaTx.scaledPicoseconds.msb) << 16);
	s->delta_rx_m = ((((int32_t)WR_DSPOR(ppi)->otherNodeDeltaRx.scaledPicoseconds.lsb) >> 16) & 0xffff) | (((int32_t)WR_DSPOR(ppi)->otherNodeDeltaRx.scaledPicoseconds.msb) << 16);

	s->delta_tx_s = ((((int32_t)WR_DSPOR(ppi)->deltaTx.scaledPicoseconds.lsb) >> 16) & 0xffff) | (((int32_t)WR_DSPOR(ppi)->deltaTx.scaledPicoseconds.msb) << 16);
	s->delta_rx_s = ((((int32_t)WR_DSPOR(ppi)->deltaRx.scaledPicoseconds.lsb) >> 16) & 0xffff) | (((int32_t)WR_DSPOR(ppi)->deltaRx.scaledPicoseconds.msb) << 16);

	cur_servo_state.delta_tx_m = (int64_t)s->delta_tx_m;
	cur_servo_state.delta_rx_m = (int64_t)s->delta_rx_m;
	cur_servo_state.delta_tx_s = (int64_t)s->delta_tx_s;
	cur_servo_state.delta_rx_s = (int64_t)s->delta_rx_s;

	/* FIXME: useful?
	strncpy(cur_servo_state.sync_source,
			  clock->netPath.ifaceName, 16);//fixme
	*/

	strcpy(cur_servo_state.slave_servo_state, "Uninitialized");

	servo_state_valid = 1;
	cur_servo_state.valid = 1;
	cur_servo_state.update_count = 0;

	got_sync = 0;
	return 0;
}

TimeInternal timeint_to_wr(TimeInternal t)
{
	TimeInternal ts;
	ts.seconds = t.seconds;
	ts.nanoseconds = t.nanoseconds;
	ts.phase = t.phase;
	return ts;
}

static int ph_adjust = 0;

int wr_servo_man_adjust_phase(int phase)
{
	ph_adjust = phase;
	return ph_adjust;
}

int wr_servo_got_sync(struct pp_instance *ppi, TimeInternal *t1,
		      TimeInternal *t2)
{
	struct wr_servo_state_t *s =
			&((struct wr_data_t *)ppi->ext_data)->servo_state;

	s->t1 = timeint_to_wr(*t1);
	s->t1.correct = 1;
	s->t2 = timeint_to_wr(*t2);
	s->t2.correct = t2->correct;

	got_sync = 1;

	return 0;
}

int wr_servo_got_delay(struct pp_instance *ppi, Integer32 cf)
{
	struct wr_servo_state_t *s =
			&((struct wr_data_t *)ppi->ext_data)->servo_state;

	s->t3 = ppi->delay_req_send_time;
	/*  s->t3.phase = 0; */
	s->t4 = timeint_to_wr(ppi->delay_req_receive_time);
	s->t4.correct = 1; /* clock->delay_req_receive_time.correct; */
	s->t4.phase = (int64_t) cf * 1000LL / 65536LL;

	return 0;
}


int wr_servo_update(struct pp_instance *ppi)
{
	struct wr_servo_state_t *s =
			&((struct wr_data_t *)ppi->ext_data)->servo_state;

	uint64_t tics;
	uint64_t big_delta_fix;
	uint64_t delay_ms_fix;

	TimeInternal ts_offset, ts_offset_hw /*, ts_phase_adjust */;

	if(!got_sync)
		return 0;

    if(!s->t1.correct || !s->t2.correct || !s->t3.correct || !s->t4.correct)
    {
	PP_VPRINTF( "servo: TimestampsIncorrect: %d %d %d %d\n",
		s->t1.correct, s->t2.correct, s->t3.correct, s->t4.correct);

	 return 0;
    }

	cur_servo_state.update_count++;

	got_sync = 0;

	if (0) { /* enable for debugging */
		dump_timestamp("servo:t1", s->t1);
		dump_timestamp("servo:t2", s->t2);
		dump_timestamp("servo:t3", s->t3);
		dump_timestamp("servo:t4", s->t4);

		dump_timestamp("->mdelay", s->mu);

	}

	s->mu = ts_sub(ts_sub(s->t4, s->t1), ts_sub(s->t3, s->t2));

	big_delta_fix =  s->delta_tx_m + s->delta_tx_s
		       + s->delta_rx_m + s->delta_rx_s;

	delay_ms_fix = (((int64_t)(ts_to_picos(s->mu) - big_delta_fix) * (int64_t) s->fiber_fix_alpha) >> FIX_ALPHA_FRACBITS)
		+ ((ts_to_picos(s->mu) - big_delta_fix) >> 1)
		+ s->delta_tx_m + s->delta_rx_s + ph_adjust;

	ts_offset = ts_add(ts_sub(s->t1, s->t2), picos_to_ts(delay_ms_fix));
	ts_offset_hw = ts_hardwarize(ts_offset, s->clock_period_ps);

	cur_servo_state.mu = (uint64_t)ts_to_picos(s->mu);
	cur_servo_state.cur_offset = ts_to_picos(ts_offset);

	cur_servo_state.delay_ms = delay_ms_fix;
	cur_servo_state.total_asymmetry =
		(cur_servo_state.mu - 2LL * (int64_t)delay_ms_fix);
	cur_servo_state.fiber_asymmetry =
		cur_servo_state.total_asymmetry
		- (s->delta_tx_m + s->delta_rx_s)
		+ (s->delta_rx_m + s->delta_tx_s);

	cur_servo_state.tracking_enabled = tracking_enabled;

	s->delta_ms = delay_ms_fix;

	tics = wr_timer_get_msec_tics();

	if (s->state != WR_WAIT_SYNC_IDLE) {
		PP_PRINTF("servo:state: %d (%s)\n", s->state,
			servo_state_str[s->state]);

		PP_PRINTF("Round-trip time (mu):      "); PP_PRINTF("%d ps\n", (int32_t)(cur_servo_state.mu));
		PP_PRINTF("Master-slave delay:        "); PP_PRINTF("%d ps\n", (int32_t)(cur_servo_state.delay_ms));
		PP_PRINTF("Master PHY delays:         "); PP_PRINTF("TX: %d ps, RX: %d ps\n", (int32_t)cur_servo_state.delta_tx_m, (int32_t)cur_servo_state.delta_rx_m);
		PP_PRINTF("Slave PHY delays:          "); PP_PRINTF("TX: %d ps, RX: %d ps\n", (int32_t)cur_servo_state.delta_tx_s, (int32_t)cur_servo_state.delta_rx_s);
		PP_PRINTF("Total link asymmetry:      "); PP_PRINTF("%d ps\n", (int32_t)(cur_servo_state.total_asymmetry));
		PP_PRINTF("Cable rtt delay:           "); PP_PRINTF("%d ps\n",
									(int32_t)(cur_servo_state.mu) - (int32_t)cur_servo_state.delta_tx_m - (int32_t)cur_servo_state.delta_rx_m -
									(int32_t)cur_servo_state.delta_tx_s - (int32_t)cur_servo_state.delta_rx_s);
		PP_PRINTF("Clock offset:              "); PP_PRINTF("%d ps\n", (int32_t)(cur_servo_state.cur_offset));
		PP_PRINTF("Phase setpoint:            "); PP_PRINTF("%d ps\n", (int32_t)(cur_servo_state.cur_setpoint));
		PP_PRINTF("Skew:                      "); PP_PRINTF("%d ps\n", (int32_t)(cur_servo_state.cur_skew));
		PP_PRINTF("Update counter:            "); PP_PRINTF("%d \n", (int32_t)(cur_servo_state.update_count));

	}

	if (wr_locking_poll(ppi) != WR_SPLL_READY)
	{
		PP_PRINTF("PLL OutOfLock, should restart sync\n");
		wr_enable_timing_output(ppi, 0);
		/* TODO check
		 * DSPOR(ppi)->doRestart = TRUE; */
	}
	
	switch(s->state)
	{
        case WR_WAIT_SYNC_IDLE:
		if(!wr_adjust_in_progress())
		{
		   s->state = s->next_state;
		}else PP_PRINTF("servo:busy\n");

		break;

	case WR_SYNC_TAI:
		wr_enable_timing_output(ppi, 0);

		if(ts_offset_hw.seconds != 0)
		{
			strcpy(cur_servo_state.slave_servo_state, "SYNC_SEC");
			wr_adjust_counters(ts_offset_hw.seconds, 0);
			wr_adjust_phase(0);

			s->next_state = WR_SYNC_NSEC;
			s->state = WR_WAIT_SYNC_IDLE;
			s->last_tics = tics;

		} else s->state = WR_SYNC_NSEC;
		break;

	case WR_SYNC_NSEC:
		strcpy(cur_servo_state.slave_servo_state, "SYNC_NSEC");

		if(ts_offset_hw.nanoseconds != 0)
		{
			wr_adjust_counters(0, ts_offset_hw.nanoseconds);

			s->next_state = WR_SYNC_NSEC;
			s->state = WR_WAIT_SYNC_IDLE;
			s->last_tics = tics;

		} else s->state = WR_SYNC_PHASE;
		break;

	case WR_SYNC_PHASE:
		strcpy(cur_servo_state.slave_servo_state, "SYNC_PHASE");
		s->cur_setpoint = ts_offset_hw.phase + ts_offset_hw.nanoseconds * 1000;

		wr_adjust_phase(s->cur_setpoint);

		s->next_state = WR_WAIT_OFFSET_STABLE;
		s->state = WR_WAIT_SYNC_IDLE;
		s->last_tics = tics;
		s->delta_ms_prev = s->delta_ms;
    	break;

    case WR_WAIT_OFFSET_STABLE:
    {
        int64_t remaining_offset = abs(ts_to_picos(ts_offset_hw));

				if(ts_offset_hw.seconds !=0 || ts_offset_hw.nanoseconds != 0)
					s->state = WR_SYNC_TAI;
        else if(remaining_offset < WR_SERVO_OFFSET_STABILITY_THRESHOLD)
        {
            wr_enable_timing_output(ppi, 1);
            s->state = WR_TRACK_PHASE;
        } else s->missed_iters++;
        
        if(s->missed_iters >= 10)
        	s->state = WR_SYNC_TAI;

        break;
    }

	case WR_TRACK_PHASE:
		strcpy(cur_servo_state.slave_servo_state, "TRACK_PHASE");
		cur_servo_state.cur_setpoint = s->cur_setpoint;
		cur_servo_state.cur_skew = s->delta_ms - s->delta_ms_prev;

		if(ts_offset_hw.seconds !=0 || ts_offset_hw.nanoseconds != 0)
				s->state = WR_SYNC_TAI;

		if(tracking_enabled)
		{
//         	shw_pps_gen_enable_output(1);
			// just follow the changes of deltaMS
			s->cur_setpoint += (s->delta_ms - s->delta_ms_prev);

			wr_adjust_phase(s->cur_setpoint);

			s->delta_ms_prev = s->delta_ms;
			s->next_state = WR_TRACK_PHASE;
			s->state = WR_WAIT_SYNC_IDLE;
			s->last_tics = tics;
		}
		break;

	}
	return 0;
}
