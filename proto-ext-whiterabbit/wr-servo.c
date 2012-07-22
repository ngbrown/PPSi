// this is a simple, PTP-like synchronization test program
// usage: netif_test [-m/-s] interface. (-m = master, -s = slave)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

//#include <inttypes.h>  -- now in ptpd-wrappers.h
#include <sys/time.h>

#include "ptpd_netif.h"
#include "hal_exports.h"
#include "ptpd_exports.h"

#include "ptpd.h"

#define WR_SYNC_NSEC 1
#define WR_SYNC_TAI 2
#define WR_SYNC_PHASE 3
#define WR_TRACK_PHASE 4
#define WR_WAIT_SYNC_IDLE 5
#define WR_WAIT_OFFSET_STABLE 6

#define WR_SERVO_OFFSET_STABILITY_THRESHOLD 60 /* psec */

/* my own timestamp arithmetic functions */

int servo_state_valid = 0;
ptpdexp_sync_state_t cur_servo_state;

static int tracking_enabled = 1;

void wr_servo_enable_tracking(int enable)
{
	tracking_enabled = enable;
}


static void dump_timestamp(char *what, wr_timestamp_t ts)
{
	fprintf(stderr, "%s = %d:%d:%d\n", what, (int32_t)ts.sec, ts.nsec, ts.phase);
}

static int64_t ts_to_picos(wr_timestamp_t ts)
{
	return (int64_t) ts.sec * (int64_t)1000000000000LL
		+ (int64_t) ts.nsec * (int64_t)1000LL
		+ (int64_t) ts.phase;
}

static wr_timestamp_t picos_to_ts(int64_t picos)
{
	int64_t phase = picos % 1000LL;
	picos = (picos - phase) / 1000LL;

	int64_t nsec = picos % 1000000000LL;
	picos = (picos-nsec)/1000000000LL;

	wr_timestamp_t ts;
	ts.sec = (int64_t) picos;
	ts.nsec = (int32_t) nsec;
	ts.phase = (int32_t) phase;

	return ts;
}

static wr_timestamp_t ts_add(wr_timestamp_t a, wr_timestamp_t b)
{
	wr_timestamp_t c;

	c.sec = 0;
	c.nsec = 0;

	c.phase =a.phase + b.phase;

	while(c.phase >= 1000)
	{
		c.phase -= 1000;
		c.nsec++;
	}

	c.nsec += (a.nsec + b.nsec);

	while(c.nsec >= 1000000000L)
	{
		c.nsec -= 1000000000L;
		c.sec++;
	}

	c.sec += (a.sec + b.sec);

	return c;
}

static wr_timestamp_t ts_sub(wr_timestamp_t a, wr_timestamp_t b)
{
	wr_timestamp_t c;

	c.sec = 0;
	c.nsec = 0;

	c.phase = a.phase - b.phase;

	while(c.phase < 0)
	{
		c.phase+=1000;
		c.nsec--;
	}

	c.nsec += a.nsec - b.nsec;
	while(c.nsec < 0)
	{
		c.nsec += 1000000000L;
		c.sec--;
	}

	c.sec += a.sec - b.sec;

	return c;
}

// "Hardwarizes" the timestamp - e.g. makes the nanosecond field a multiple
// of 8ns cycles and puts the extra nanoseconds in the phase field
static wr_timestamp_t ts_hardwarize(wr_timestamp_t ts, int clock_period_ps)
{
    int32_t q_threshold;

    q_threshold = (clock_period_ps + 999) / 1000;

	if(ts.nsec > 0)
	{
		int32_t extra_nsec = ts.nsec % q_threshold;

		if(extra_nsec)
		{
			ts.nsec -=extra_nsec;
			ts.phase += extra_nsec * 1000;
		}
	}

	if(ts.nsec < 0) {
	 	ts.nsec += 1000000000LL;
	 	ts.sec--;
	}

    if(ts.sec == -1 && ts.nsec > 0)
    {
        ts.sec++;
        ts.nsec -= 1000000000LL;
    }

    if(ts.nsec < 0 && ts.nsec >= (-q_threshold) && ts.sec == 0)
    {
        ts.nsec += q_threshold;
        ts.phase -= q_threshold * 1000;
    }

	return ts;
}

static int got_sync = 0;

void wr_servo_reset()
{
//    pps_gen_enable_output(0); /* fixme: unportable */
	cur_servo_state.valid = 0;
	servo_state_valid = 0;
//    ptpd_netif_enable_timing_output(0);

}

int wr_servo_init(PtpPortDS *clock)
{
	wr_servo_state_t *s = &clock->wr_servo;

	/* Determine the alpha coefficient */
	if(ptpd_netif_read_calibration_data(clock->netPath.ifaceName, NULL, NULL, &s->fiber_fix_alpha, &s->clock_period_ps) != PTPD_NETIF_OK)
		return -1;

    ptpd_netif_enable_timing_output(0);

	strncpy(s->if_name, clock->netPath.ifaceName, 16);

	s->state = WR_SYNC_TAI;
	s->cur_setpoint = 0;
	s->missed_iters = 0;
	
	s->delta_tx_m = ((((int32_t)clock->otherNodeDeltaTx.scaledPicoseconds.lsb) >> 16) & 0xffff) | (((int32_t)clock->otherNodeDeltaTx.scaledPicoseconds.msb) << 16);
	s->delta_rx_m = ((((int32_t)clock->otherNodeDeltaRx.scaledPicoseconds.lsb) >> 16) & 0xffff) | (((int32_t)clock->otherNodeDeltaRx.scaledPicoseconds.msb) << 16);

	s->delta_tx_s = ((((int32_t)clock->deltaTx.scaledPicoseconds.lsb) >> 16) & 0xffff) | (((int32_t)clock->deltaTx.scaledPicoseconds.msb) << 16);
	s->delta_rx_s = ((((int32_t)clock->deltaRx.scaledPicoseconds.lsb) >> 16) & 0xffff) | (((int32_t)clock->deltaRx.scaledPicoseconds.msb) << 16);

    fprintf(stderr,"servo:pre: %d/%dps\n", clock->deltaRx.scaledPicoseconds.lsb,clock->deltaRx.scaledPicoseconds.msb);

	fprintf(stderr,"servo:Deltas: Master: Tx=%dps, Tx=%dps, Slave: tx=%dps, Rx=%dps.\n",
	s->delta_tx_m,
	s->delta_rx_m,
	s->delta_tx_s,
	s->delta_rx_s);

	cur_servo_state.delta_tx_m = (int64_t)s->delta_tx_m;
	cur_servo_state.delta_rx_m = (int64_t)s->delta_rx_m;
	cur_servo_state.delta_tx_s = (int64_t)s->delta_tx_s;
	cur_servo_state.delta_rx_s = (int64_t)s->delta_rx_s;

	strncpy(cur_servo_state.sync_source,
			  clock->netPath.ifaceName, 16);//fixme
	strncpy(cur_servo_state.slave_servo_state,
			  "Uninitialized", 32);

	servo_state_valid = 1;
	cur_servo_state.valid = 1;
	cur_servo_state.update_count = 0;

	got_sync = 0;
	return 0;
}

wr_timestamp_t timeint_to_wr(TimeInternal t)
{
	wr_timestamp_t ts;
	ts.sec = t.seconds;
	ts.nsec = t.nanoseconds;
	ts.phase = t.phase;
	return ts;
}

static int ph_adjust = 0;

int wr_servo_man_adjust_phase(int phase)
{
	ph_adjust = phase;
	return ph_adjust;
}

int wr_servo_got_sync(PtpPortDS *clock, TimeInternal t1, TimeInternal t2)
{
	wr_servo_state_t *s = &clock->wr_servo;

	s->t1 = timeint_to_wr(t1);
	s->t1.correct = 1;
	s->t2 = timeint_to_wr(t2);
	s->t2.correct = t2.correct;

	got_sync = 1;

	return 0;
}

int wr_servo_got_delay(PtpPortDS *clock, Integer32 cf)
{
	wr_servo_state_t *s = &clock->wr_servo;

	s->t3 = clock->delayReq_tx_ts;
	//  s->t3.phase = 0;
	s->t4 = timeint_to_wr(clock->delay_req_receive_time);
	s->t4.correct = 1; //clock->delay_req_receive_time.correct;
	s->t4.phase = (int64_t) cf * 1000LL / 65536LL;
	return 0;
}


int wr_servo_update(PtpPortDS *clock)
{
	wr_servo_state_t *s = &clock->wr_servo;

	uint64_t tics;
	uint64_t big_delta_fix;
	uint64_t delay_ms_fix;

	wr_timestamp_t ts_offset, ts_offset_hw /*, ts_phase_adjust */;

	if(!got_sync)
		return 0;

    if(!s->t1.correct || !s->t2.correct || !s->t3.correct || !s->t4.correct)
    {
       	PTPD_TRACE(TRACE_SERVO, NULL, "servo: TimestampsIncorrect: %d %d %d %d\n",
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

	tics = ptpd_netif_get_msec_tics();

	PTPD_TRACE(TRACE_SERVO, NULL, "servo:HWOffset", ts_offset_hw);
	PTPD_TRACE(TRACE_SERVO, NULL, "servo:state: %d\n", s->state);


	if(ptpd_netif_locking_poll(0, clock->netPath.ifaceName, 0) != PTPD_NETIF_READY)
	{
		PTPD_TRACE(TRACE_SERVO, NULL, "PLL OutOfLock, restarting sync\n");
    ptpd_netif_enable_timing_output(0);
		clock->doRestart = TRUE;
	}
	
	switch(s->state)
	{
        case WR_WAIT_SYNC_IDLE:

		if(!ptpd_netif_adjust_in_progress())
		{
		   s->state = s->next_state;
		}else fprintf(stderr,"servo:busy\n");

		break;

	case WR_SYNC_TAI:
    ptpd_netif_enable_timing_output(0);

		if(ts_offset_hw.sec != 0)
		{
			strcpy(cur_servo_state.slave_servo_state, "SYNC_SEC");
            ptpd_netif_adjust_counters(ts_offset_hw.sec, 0);
            ptpd_netif_adjust_phase(0);

			s->next_state = WR_SYNC_NSEC;
			s->state = WR_WAIT_SYNC_IDLE;
			s->last_tics = tics;

		} else s->state = WR_SYNC_NSEC;
		break;

	case WR_SYNC_NSEC:
		strcpy(cur_servo_state.slave_servo_state, "SYNC_NSEC");

		if(ts_offset_hw.nsec != 0)
		{
			ptpd_netif_adjust_counters(0, ts_offset_hw.nsec);

			s->next_state = WR_SYNC_NSEC;
			s->state = WR_WAIT_SYNC_IDLE;
			s->last_tics = tics;

		} else s->state = WR_SYNC_PHASE;
		break;

	case WR_SYNC_PHASE:
		strcpy(cur_servo_state.slave_servo_state, "SYNC_PHASE");
		s->cur_setpoint = ts_offset_hw.phase + ts_offset_hw.nsec * 1000;

        ptpd_netif_adjust_phase(s->cur_setpoint);

		s->next_state = WR_WAIT_OFFSET_STABLE;
        s->state = WR_WAIT_SYNC_IDLE;
		s->last_tics = tics;
		s->delta_ms_prev = s->delta_ms;
    	break;

    case WR_WAIT_OFFSET_STABLE:
    {
        int64_t remaining_offset = abs(ts_to_picos(ts_offset_hw));

				if(ts_offset_hw.sec !=0 || ts_offset_hw.nsec != 0)
					s->state = WR_SYNC_TAI;
        else if(remaining_offset < WR_SERVO_OFFSET_STABILITY_THRESHOLD)
        {
            ptpd_netif_enable_timing_output(1);
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

		if(ts_offset_hw.sec !=0 || ts_offset_hw.nsec != 0)
				s->state = WR_SYNC_TAI;

		if(tracking_enabled)
		{
//         	pps_gen_enable_output(1);
			// just follow the changes of deltaMS
			s->cur_setpoint += (s->delta_ms - s->delta_ms_prev);

		   	ptpd_netif_adjust_phase(s->cur_setpoint);

			s->delta_ms_prev = s->delta_ms;
			s->next_state = WR_TRACK_PHASE;
			s->state = WR_WAIT_SYNC_IDLE;
			s->last_tics = tics;
		}
		break;

	}
	return 0;
}


