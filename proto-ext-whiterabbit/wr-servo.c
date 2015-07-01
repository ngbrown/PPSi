#include <ppsi/ppsi.h>
#include "wr-api.h"
#include <libwr/shmem.h>

#define WR_SERVO_NONE 0
#define WR_SYNC_NSEC 1
#define WR_SYNC_TAI 2
#define WR_SYNC_PHASE 3
#define WR_TRACK_PHASE 4
#define WR_WAIT_OFFSET_STABLE 5

#define WR_SERVO_OFFSET_STABILITY_THRESHOLD 60 /* psec */

#define FIX_ALPHA_FRACBITS 40

/* Define threshold values for SNMP */
/* TODO: These values need to be tuned! */
#define SNMP_MAX_OFFSET 1000000
#define SNMP_MAX_DELTA_RTT 1000000
#define SNMP_MAX_RXTX_DELTAS 1000000

static const char *servo_name[] = {
	[WR_SERVO_NONE] = "Uninitialized",
	[WR_SYNC_NSEC] = "SYNC_NSEC",
	[WR_SYNC_TAI] = "SYNC_SEC",
	[WR_SYNC_PHASE] = "SYNC_PHASE",
	[WR_TRACK_PHASE] = "TRACK_PHASE",
	[WR_WAIT_OFFSET_STABLE] = "WAIT_OFFSET_STABLE",
};

static int tracking_enabled = 1; /* FIXME: why? */
extern struct wrs_shm_head *ppsi_head;
static struct wr_servo_state *saved_servo_pointer; /* required for
						* wr_servo_reset, which doesn't
						* have ppi context. */

void wr_servo_enable_tracking(int enable)
{
	tracking_enabled = enable;
}

/* my own timestamp arithmetic functions */

static void dump_timestamp(struct pp_instance *ppi, char *what, TimeInternal ts)
{
	pp_diag(ppi, servo, 2, "%s = %d:%d:%d\n", what, (int32_t)ts.seconds,
		  ts.nanoseconds, ts.phase);
}

static int64_t ts_to_picos(TimeInternal ts)
{
	return ts.seconds * 1000000000000LL
		+ ts.nanoseconds * 1000LL
		+ ts.phase;
}

static TimeInternal picos_to_ts(int64_t picos)
{
	uint64_t nsec, phase;
	TimeInternal ts;

	nsec = picos;
	phase = __div64_32(&nsec, 1000);

	ts.nanoseconds = __div64_32(&nsec, PP_NSEC_PER_SEC);
	ts.seconds = nsec; /* after the division */
	ts.phase = phase;
	return ts;
}

static TimeInternal ts_add(TimeInternal a, TimeInternal b)
{
	TimeInternal c;

	c.phase = a.phase + b.phase;
	c.nanoseconds = a.nanoseconds + b.nanoseconds;
	c.seconds = a.seconds + b.seconds;

	while (c.phase >= 1000) {
		c.phase -= 1000;
		c.nanoseconds++;
	}
	while (c.nanoseconds >= PP_NSEC_PER_SEC) {
		c.nanoseconds -= PP_NSEC_PER_SEC;
		c.seconds++;
	}
	return c;
}

static TimeInternal ts_sub(TimeInternal a, TimeInternal b)
{
	TimeInternal c;

	c.phase = a.phase - b.phase;
	c.nanoseconds = a.nanoseconds - b.nanoseconds;
	c.seconds = a.seconds - b.seconds;

	while(c.phase < 0) {
		c.phase += 1000;
		c.nanoseconds--;
	}
	while(c.nanoseconds < 0) {
		c.nanoseconds += PP_NSEC_PER_SEC;
		c.seconds--;
	}
	return c;
}

/* "Hardwarizes" the timestamp - e.g. makes the nanosecond field a multiple
 * of 8ns cycles and puts the extra nanoseconds in the phase field */
static TimeInternal ts_hardwarize(TimeInternal ts, int clock_period_ps)
{
	int32_t q_threshold;

	q_threshold = (clock_period_ps + 999) / 1000;

	if (ts.nanoseconds > 0) {
		int32_t extra_nsec = ts.nanoseconds % q_threshold;

		if(extra_nsec) {
			ts.nanoseconds -= extra_nsec;
			ts.phase += extra_nsec * 1000;
		}
	}

	if (ts.nanoseconds < 0) {
		ts.nanoseconds += PP_NSEC_PER_SEC;
		ts.seconds--;
	}

	if (ts.seconds == -1 && ts.nanoseconds > 0) {
		ts.seconds++;
		ts.nanoseconds -= PP_NSEC_PER_SEC;
	}

	if (ts.nanoseconds < 0 && ts.nanoseconds >= (-q_threshold)
	    && ts.seconds == 0) {
		ts.nanoseconds += q_threshold;
		ts.phase -= q_threshold * 1000;
	}

	return ts;
}

/* end my own timestamp arithmetic functions */

static int got_sync = 0;

void wr_servo_reset(void)
{
	if (saved_servo_pointer)
		saved_servo_pointer->flags = 0;
}

static inline int32_t delta_to_ps(struct FixedDelta d)
{
	UInteger64 *sps = &d.scaledPicoseconds; /* ieee type :( */

	return (sps->lsb >> 16) | (sps->msb << 16);
}

int wr_servo_init(struct pp_instance *ppi)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);
	struct wr_servo_state *s =
			&((struct wr_data *)ppi->ext_data)->servo_state;
	/* shmem lock */
	wrs_shm_write(ppsi_head, WRS_SHM_WRITE_BEGIN);
	/* Determine the alpha coefficient */
	if (wrp->ops->read_calib_data(ppi, 0, 0,
		&s->fiber_fix_alpha, &s->clock_period_ps) != WR_HW_CALIB_OK)
		return -1;

	wrp->ops->enable_timing_output(ppi, 0);

	strncpy(s->if_name, ppi->cfg.iface_name, sizeof(s->if_name));
	s->state = WR_SERVO_NONE; /* Turned into SYNC_TAI at 1st iteration */
	s->cur_setpoint = 0;
	s->missed_iters = 0;

	s->delta_tx_m = delta_to_ps(wrp->otherNodeDeltaTx);
	s->delta_rx_m = delta_to_ps(wrp->otherNodeDeltaRx);
	s->delta_tx_s = delta_to_ps(wrp->deltaTx);
	s->delta_rx_s = delta_to_ps(wrp->deltaRx);

	strcpy(s->servo_state_name, "Uninitialized");

	saved_servo_pointer = s;
	saved_servo_pointer->flags |= WR_FLAG_VALID;
	s->update_count = 0;

	got_sync = 0;

	/* shmem unlock */
	wrs_shm_write(ppsi_head, WRS_SHM_WRITE_END);
	return 0;
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
	struct wr_servo_state *s =
			&((struct wr_data *)ppi->ext_data)->servo_state;

	s->t1 = *t1;
	s->t1.correct = 1;
	s->t2 = *t2;

	got_sync = 1;

	return 0;
}

int wr_servo_got_delay(struct pp_instance *ppi, Integer32 cf)
{
	struct wr_servo_state *s =
			&((struct wr_data *)ppi->ext_data)->servo_state;

	s->t3 = ppi->t3;
	/*  s->t3.phase = 0; */
	s->t4 = ppi->t4;
	s->t4.correct = 1; /* clock->delay_req_receive_time.correct; */
	s->t4.phase = (int64_t) cf * 1000LL / 65536LL;

	return 0;
}


int wr_servo_update(struct pp_instance *ppi)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);
	struct wr_servo_state *s =
			&((struct wr_data *)ppi->ext_data)->servo_state;

	uint64_t big_delta_fix;
	uint64_t delay_ms_fix;
	static int errcount;
	int remaining_offset;
	int64_t picos_mu_prev = 0;
	TimeInternal ts_offset, ts_offset_hw /*, ts_phase_adjust */;

	if(!got_sync)
		return 0;

	if(!s->t1.correct || !s->t2.correct ||
	   !s->t3.correct || !s->t4.correct) {
		errcount++;
		if (errcount > 5) { /* a 2-3 in a row are expected */
			pp_error("%s: TimestampsIncorrect: %d %d %d %d\n",
				 __func__, s->t1.correct, s->t2.correct,
				 s->t3.correct, s->t4.correct);
			errcount = 0;
		}
		return 0;
	}

	/* shmem lock */
	wrs_shm_write(ppsi_head, WRS_SHM_WRITE_BEGIN);

	s->update_count++;

	got_sync = 0;

	s->mu = ts_sub(ts_sub(s->t4, s->t1), ts_sub(s->t3, s->t2));

	if (__PP_DIAG_ALLOW(ppi, pp_dt_servo, 1)) {
		dump_timestamp(ppi, "servo:t1", s->t1);
		dump_timestamp(ppi, "servo:t2", s->t2);
		dump_timestamp(ppi, "servo:t3", s->t3);
		dump_timestamp(ppi, "servo:t4", s->t4);
		dump_timestamp(ppi, "->mdelay", s->mu);
	}
	picos_mu_prev = s->picos_mu;
	s->picos_mu = ts_to_picos(s->mu);
	big_delta_fix =  s->delta_tx_m + s->delta_tx_s
		       + s->delta_rx_m + s->delta_rx_s;

	if ((signed)s->picos_mu < (signed)big_delta_fix) {
		/* avoid negatives in calculations */
		s->picos_mu = big_delta_fix;
	}

	delay_ms_fix = (((int64_t)(s->picos_mu - big_delta_fix) * (int64_t) s->fiber_fix_alpha) >> FIX_ALPHA_FRACBITS)
		+ ((s->picos_mu - big_delta_fix) >> 1)
		+ s->delta_tx_m + s->delta_rx_s + ph_adjust;

	ts_offset = ts_add(ts_sub(s->t1, s->t2), picos_to_ts(delay_ms_fix));
	ts_offset_hw = ts_hardwarize(ts_offset, s->clock_period_ps);

	/* is it possible to calculate it in client,
	 * but then t1 and t2 require shmem locks */
	s->offset = ts_to_picos(ts_offset);

	s->tracking_enabled =  tracking_enabled;

	s->delta_ms = delay_ms_fix;

	if (wrp->ops->locking_poll(ppi, 0) != WR_SPLL_READY) {
		pp_diag(ppi, servo, 1, "PLL OutOfLock, should restart sync\n");
		wrp->ops->enable_timing_output(ppi, 0);
		/* TODO check
		 * DSPOR(ppi)->doRestart = TRUE; */
	}

	if (s->state == WR_SERVO_NONE)
		s->state = WR_SYNC_TAI;
	if (s->state ==WR_SYNC_TAI) /* unsynced: no PPS output */
		wrp->ops->enable_timing_output(ppi, 0);
	if (s->state == WR_SYNC_TAI && ts_offset_hw.seconds == 0)
		s->state = WR_SYNC_NSEC;
	if (s->state == WR_SYNC_NSEC && ts_offset_hw.nanoseconds == 0)
		s->state = WR_SYNC_PHASE;

	pp_diag(ppi, servo, 2, "offset_hw: %li.%09li\n",
		(long)ts_offset_hw.seconds, (long)ts_offset_hw.nanoseconds);

	pp_diag(ppi, servo, 1, "wr_servo state: %s%s\n",
		servo_name[s->state],
		s->flags & WR_FLAG_WAIT_HW ? " (wait for hw)" : "");

	/*
	 * After each action on the hardware, we must verify if it is over.
	 * However, we loose one iteration every two. To be audited later.
	 */
	if (s->flags & WR_FLAG_WAIT_HW) {
		if (!wrp->ops->adjust_in_progress())
			s->flags &= ~WR_FLAG_WAIT_HW;
		else
			pp_diag(ppi, servo, 1, "servo:busy\n");
		goto out;
	}

	switch (s->state) {
	case WR_SYNC_TAI:
		wrp->ops->adjust_counters(ts_offset_hw.seconds, 0);
		wrp->ops->adjust_phase(0);
		s->flags |= WR_FLAG_WAIT_HW;
		s->state = WR_SYNC_NSEC;
		break;

	case WR_SYNC_NSEC:
		wrp->ops->adjust_counters(0, ts_offset_hw.nanoseconds);
		s->flags |= WR_FLAG_WAIT_HW;
		s->state = WR_SYNC_PHASE;
		break;

	case WR_SYNC_PHASE:
		s->cur_setpoint = ts_offset_hw.phase
			+ ts_offset_hw.nanoseconds * 1000;

		wrp->ops->adjust_phase(s->cur_setpoint);

		s->flags |= WR_FLAG_WAIT_HW;
		s->state = WR_WAIT_OFFSET_STABLE;
		s->delta_ms_prev = s->delta_ms;
		break;

	case WR_WAIT_OFFSET_STABLE:

		if (ts_offset_hw.seconds || ts_offset_hw.nanoseconds) {
			s->state = WR_SYNC_TAI;
			break;
		}
		/* ts_to_picos() below returns phase alone */
		remaining_offset = abs(ts_to_picos(ts_offset_hw));
		if(remaining_offset < WR_SERVO_OFFSET_STABILITY_THRESHOLD) {
			wrp->ops->enable_timing_output(ppi, 1);
			s->state = WR_TRACK_PHASE;
		} else {
			s->missed_iters++;
		}
		if (s->missed_iters >= 10) {
			s->missed_iters = 0;
			s->state = WR_SYNC_TAI;
		}
		break;

	case WR_TRACK_PHASE:
		s->skew = s->delta_ms - s->delta_ms_prev;

		if (ts_offset_hw.seconds || ts_offset_hw.nanoseconds) {
				s->state = WR_SYNC_TAI;
				break;
		}

		if(tracking_enabled) {
			// just follow the changes of deltaMS
			s->cur_setpoint += (s->delta_ms - s->delta_ms_prev);

			wrp->ops->adjust_phase(s->cur_setpoint);
			pp_diag(ppi, time, 1, "adjust phase %i\n",
				s->cur_setpoint);

			s->delta_ms_prev = s->delta_ms;
			s->flags |= WR_FLAG_WAIT_HW;
			s->state = WR_TRACK_PHASE;
		}
		break;

	}
	/* update string state name */
	strcpy(s->servo_state_name, servo_name[s->state]);

	/* Increase number of servo updates with state different than
	 * WR_TRACK_PHASE. (Used by SNMP) */
	if (s->state != WR_TRACK_PHASE)
		s->n_err_state++;

	/* Increase number of servo updates with offset exceeded
	 * SNMP_MAX_OFFSET (Used by SNMP) */
	if (abs(s->offset) > SNMP_MAX_OFFSET)
		s->n_err_offset++;

	/* Increase number of servo updates with delta rtt exceeded
	 * SNMP_MAX_DELTA_RTT (Used by SNMP) */
	if (abs(picos_mu_prev - s->picos_mu) > SNMP_MAX_DELTA_RTT)
		s->n_err_delta_rtt++;

	/* Increase number of servo updates with delta_*x_* bigger than
	 * SNMP_MAX_RXTX_DELTAS. (Used by SNMP) */
	if ((s->delta_tx_m > SNMP_MAX_RXTX_DELTAS)
	    || (s->delta_rx_m > SNMP_MAX_RXTX_DELTAS)
	    || (s->delta_tx_s > SNMP_MAX_RXTX_DELTAS)
	    || (s->delta_rx_s > SNMP_MAX_RXTX_DELTAS))
		s->n_err_rxtx_deltas++;

out:
	/* shmem unlock */
	wrs_shm_write(ppsi_head, WRS_SHM_WRITE_END);
	return 0;
}
