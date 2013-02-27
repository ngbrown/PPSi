#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include <pps_gen.h>
#include <syscon.h>
#include <ptpd_netif.h>

static struct pp_timer wrpc_timers[PP_TIMER_ARRAY_SIZE];

int wrpc_timer_init(struct pp_instance *ppi)
{
	uint32_t i;

	for (i = 0; i < PP_TIMER_ARRAY_SIZE; i++)
		ppi->timers[i] = &wrpc_timers[i];

	return 0;
}

int wrpc_timer_start(uint32_t interval_ms, struct pp_timer *tm)
{
	tm->start = ptpd_netif_get_msec_tics();
	tm->interval_ms = interval_ms;
	return 0;
}

int wrpc_timer_stop(struct pp_timer *tm)
{
	tm->interval_ms = 0;
	tm->start = 0;

	return 0;
}

int wrpc_timer_expired(struct pp_timer *tm)
{
	uint32_t now;

	if (tm->start == 0) {
		PP_PRINTF("%s: Warning: timer %p not started\n", __func__, tm);
		return 0;
	}

	now = ptpd_netif_get_msec_tics();

	if (now > tm->start + tm->interval_ms) {
		tm->start = now;
		return 1;
	}

	return 0;
}

void wrpc_timer_adjust_all(struct pp_instance *ppi, int32_t diff)
{
	/* No need for timer adjust in SPEC: it uses a sort of monotonic
	 * tstamp for timers
	*/
}

uint32_t wrpc_timer_get_msec_tics(void)
{
	return timer_get_tics();
}

int pp_timer_init(struct pp_instance *ppi)
	__attribute__((alias("wrpc_timer_init")));

int pp_timer_start(uint32_t interval_ms, struct pp_timer *tm)
	__attribute__((alias("wrpc_timer_start")));

int pp_timer_stop(struct pp_timer *tm)
	__attribute__((alias("wrpc_timer_stop")));

int pp_timer_expired(struct pp_timer *tm)
	__attribute__((alias("wrpc_timer_expired")));

void pp_timer_adjust_all(struct pp_instance *ppi, int32_t diff)
	__attribute__((alias("wrpc_timer_adjust_all")));

uint32_t wr_timer_get_msec_tics(void)
	__attribute__((alias("wrpc_timer_get_msec_tics")));
