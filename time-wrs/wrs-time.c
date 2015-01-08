/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/timex.h>
#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>

#include <hal_exports.h>
/* FIXME: these externs are needed here because we can not include
 * hal_exports.h with HAL_EXPORT_STRUCTURES twice (the first is by
 * arch-wrs/wrs-calibration.c): structs are declared and
 * defined in .h file, so this would lead to a multiple definition. */
extern struct minipc_pd __rpcdef_pps_cmd;
extern struct minipc_pd __rpcdef_lock_cmd;

int wrs_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec)
{
	hexp_pps_params_t p;
	int cmd;
	int ret, rval;

	if (!adjust_nsec && !adjust_sec)
		return 0;

	if (adjust_sec && adjust_nsec) {
		pp_printf("FATAL: trying to adjust both the SEC and the NS"
			" counters simultaneously. \n");
		exit(1);
	}

	p.adjust_sec = adjust_sec;
	p.adjust_nsec = adjust_nsec;
	cmd = (adjust_sec ? HEXP_PPSG_CMD_ADJUST_SEC : HEXP_PPSG_CMD_ADJUST_NSEC);

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd, &rval, cmd, &p);

	if (ret < 0 || rval < 0) {
		pp_printf("%s: error (local %i remote %i)\n",
			  __func__, ret, rval);
		return -1;
	}
	return 0;
}

int wrs_adjust_phase(int32_t phase_ps)
{
	hexp_pps_params_t p;
	int ret, rval;
	p.adjust_phase_shift = phase_ps;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd,
		&rval, HEXP_PPSG_CMD_ADJUST_PHASE, &p);

	if (ret < 0)
		return ret;

	return rval;
}

int wrs_adjust_in_progress(void)
{
	hexp_pps_params_t p;
	int ret, rval;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd,
		&rval, HEXP_PPSG_CMD_POLL, &p);

	if ((ret < 0) || rval)
		return 0;

	return 1;
}

int wrs_enable_ptracker(struct pp_instance *ppi)
{
	int ret, rval;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_lock_cmd,
			  &rval, ppi->iface_name, HEXP_LOCK_CMD_ENABLE_TRACKING, 0);

	if ((ret < 0) || (rval < 0))
		return WR_SPLL_ERROR;

	return WR_SPLL_OK;
}

int wrs_enable_timing_output(struct pp_instance *ppi, int enable)
{
	int ret, rval;
	hexp_pps_params_t p;

	if (enable == WR_DSPOR(ppi)->ppsOutputOn)
		return WR_SPLL_OK;
	WR_DSPOR(ppi)->ppsOutputOn = enable;

	p.pps_valid = enable;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd,
			&rval, HEXP_PPSG_CMD_SET_VALID, &p);

	if ((ret < 0) || (rval < 0))
		return WR_SPLL_ERROR;

	return WR_SPLL_OK;
}

int wrs_locking_disable(struct pp_instance *ppi)
{
	return WR_SPLL_OK;
}

int wrs_locking_enable(struct pp_instance *ppi)
{
	int ret, rval;

	pp_diag(ppi, time, 1, "Start locking\n");

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_lock_cmd,
			  &rval, ppi->iface_name, HEXP_LOCK_CMD_START, 0);

	if ((ret < 0) || (rval < 0))
		return WR_SPLL_ERROR;

	return WR_SPLL_OK;
}

int wrs_locking_poll(struct pp_instance *ppi, int grandmaster)
{
	int ret, rval;

	if (grandmaster) /* FIXME: check wrs grandmaster PLL */
		return WR_SPLL_READY;

	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_lock_cmd,
			  &rval, ppi->iface_name, HEXP_LOCK_CMD_CHECK, 0);
	if (ret != HEXP_LOCK_STATUS_LOCKED) {
		pp_diag(ppi, time, 2, "PLL is not ready\n");
		return WR_SPLL_ERROR; /* FIXME should be WR_SPLL_NOT_READY */
	}
	pp_diag(ppi, time, 2, "PLL is locked\n");
	return WR_SPLL_READY;
}


static int wrs_time_get(struct pp_instance *ppi, TimeInternal *t)
{

	hexp_pps_params_t p;
	int cmd;
	int ret, rval;

	cmd = HEXP_PPSG_CMD_GET; /* likely not implemented... */
	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd, &rval,
					  cmd, &p);
	if (ret < 0 || rval < 0) {
		/*
		 * It failed, so fall back on unix time. Please note
		 * that these times are mainly for logging, nothing
		 * critical is there, as T1..T4 are frame stamps.
		 */
		return unix_time_ops.get(ppi, t);
	}

	/* FIXME Don't know whether p.current_phase_shift is to be assigned
	 * to t->phase or t->raw_phase. I ignore it, it's not useful here. */
	t->seconds = p.current_sec;
	t->nanoseconds = p.current_nsec;
	t->correct = p.pps_valid;

	if (!(pp_global_d_flags & PP_FLAG_NOTIMELOG))
		pp_diag(ppi, time, 2, "%s: (valid %x) %9li.%09li\n", __func__,
			p.pps_valid,
			(long)p.current_sec, (long)p.current_nsec);
	return rval;
}

static int wrs_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	TimeInternal diff;
	struct timex tx;
	int tai_offset = 0;

	/*
	 * This is almost unused in ppsi, only proto-standard/servo.c
	 * calls it, at initialization time, when the offset is bigger
	 * than one second.  Or ...
	 */
	if (!t) /* ... when the utc/tai offset changes, if t is NULL */
		return unix_time_ops.set(ppi, t);

	/*
	 * We say "weird" because we are not expected to set time here;
	 * rather, time setting goes usually from the WR servo, straight
	 * to the HAL process, where a difference is injected into the fpga;
	 * rather then "setting" an absolute time, which we can't do
	 * and thus T3 is referenced below, to get some approximate value...
	 */
	pp_diag(ppi, time, 1, "%s: (weird) %9li.%09li\n", __func__,
		(long)t->seconds, (long)t->nanoseconds);
	/* We have no way to get the WR time, currently. So use our T3 */
	sub_TimeInternal(&diff, t, &ppi->t3);

	/*
	 * We can adjust nanoseconds or seconds, but not both at the
	 * same time. When an adjustment is in progress we can't do
	 * the other.  So make nanoseconds first if > 10ms, and the
	 * servo will call us again later for the seconds part.
	 */
	if (abs(diff.nanoseconds) > 10 * 1000 * 1000) {
		pp_diag(ppi, time, 1, "%s: adjusting nanoseconds: %li\n",
			__func__, (long)diff.nanoseconds);
		diff.seconds = 0;
	} else {
		pp_diag(ppi, time, 1, "%s: adjusting seconds: %li\n",
			__func__, (long)diff.seconds);
		diff.nanoseconds = 0;
	}
	wrs_adjust_counters(diff.seconds, diff.nanoseconds);


	/* If WR time is unrelated to real-world time, we are done. */
	if (t->seconds < 1420730822 /* "now" as I write this */)
		return 0;

	/*
	 * Finally, set unix time too, but count the UTC/TAI difference
	 * assuming somebody has set up up for us
	 */
	memset(&tx, 0, sizeof(tx));
	if (adjtimex(&tx) >= 0) {
		/*
		 * Our WRS kernel has tai support, but our compiler does not.
		 * We are 32-bit only, and we know for sure that tai is
		 * exactly after stbcnt. It's a bad hack, but it works
		 */
		tai_offset = *((int *)(&tx.stbcnt) + 1);
	}
	t->seconds -= tai_offset;
	unix_time_ops.set(ppi, t);
	return 0;
}

static int wrs_time_adjust_offset(struct pp_instance *ppi, long offset_ns)
{
	pp_diag(ppi, time, 1, "adjust offset %09li\n", offset_ns);
	return wrs_adjust_counters(0, offset_ns);
}

static int wrs_time_adjust_freq(struct pp_instance *ppi, long freq_ppb)
{
	static void *mapaddress;
	long tmp;
	int32_t regval;
	int fd;
	/*
	 * This is only called by the non-wr servo. So the softpll is off
	 * and we can just touch the DAC directly
	 */
	if (!mapaddress) {
		/* FIXME: we should call the wrs library instead */
		if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
			fprintf(stderr, "ppsi: /dev/mem: %s\n",
				strerror(errno));
			exit(1);
		}
		mapaddress = mmap(0, 0x1000, PROT_READ | PROT_WRITE,
				  MAP_SHARED, fd, 0x10010000);
		close(fd);

		if (mapaddress == MAP_FAILED) {
			fprintf(stderr, "ppsi: mmap(/dev/mem): %s\n",
				strerror(errno));
			exit(1);
		}
	}
	/*
	 * Our DAC changes the freqency by 1.2 ppm every 0x1000 counts.
	 * We have 16 bits (the high bits select which DAC to control: 0 ok
	 */
	tmp = freq_ppb * 0x1000 / 1200;
	if (tmp > 0x7fff)
		tmp = 0x7fff;
	if (tmp < -0x8000)
		tmp = -0x8000;
	regval = 0x8000 + tmp; /* 0x8000 is the mid-range, nominal value */
	pp_diag(ppi, time, 1, "Set frequency: 0x%04x (ppb = %li)\n", regval,
		freq_ppb);

	*(uint32_t *)(mapaddress + 0x144) = regval;
	return 0;
}

static int wrs_time_adjust(struct pp_instance *ppi, long offset_ns,
			   long freq_ppb)
{
	if (freq_ppb != 0)
		pp_diag(ppi, time, 1, "Warning: %s: can not adjust freq_ppb %li\n",
				__func__, freq_ppb);
	return wrs_time_adjust_offset(ppi, offset_ns);
}

static unsigned long wrs_calc_timeout(struct pp_instance *ppi,
				      int millisec)
{
	/* We can rely on unix's CLOCK_MONOTONIC timing for timeouts */
	return unix_time_ops.calc_timeout(ppi, millisec);
}

struct pp_time_operations wrs_time_ops = {
	.get = wrs_time_get,
	.set = wrs_time_set,
	.adjust = wrs_time_adjust,
	.adjust_offset = wrs_time_adjust_offset,
	.adjust_freq = wrs_time_adjust_freq,
	.calc_timeout = wrs_calc_timeout,
};
