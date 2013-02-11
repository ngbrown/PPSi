#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "bare-x86-64.h"
#include <unistd.h>

static struct pp_timer bare_timers[PP_TIMER_ARRAY_SIZE];

int bare_timer_init(struct pp_instance *ppi)
{
	uint32_t i;

	for (i = 0; i < PP_TIMER_ARRAY_SIZE; i++)
		ppi->timers[i] = &bare_timers[i];

	return 0;
}

int bare_timer_start(uint32_t interval_ms, struct pp_timer *tm)
{
	struct bare_timeval now;
	sys_clock_gettime(CLOCK_MONOTONIC, &now);
	tm->start = ((uint64_t)(now.tv_sec)) * 1000 +
			       (now.tv_nsec / 1000000);
	tm->interval_ms = interval_ms;
	return 0;
}

int bare_timer_stop(struct pp_timer *tm)
{
	tm->interval_ms = 0;
	tm->start = 0;

	return 0;
}

int bare_timer_expired(struct pp_timer *tm)
{
	struct bare_timeval now;
	uint64_t now_ms;

	if (tm->start == 0) {
		PP_PRINTF("%s: Warning: timer %p not started\n", __func__, tm);
		return 0;
	}

	sys_clock_gettime(CLOCK_MONOTONIC, &now);
	now_ms = ((uint64_t)(now.tv_sec)) * 1000 +
			   (now.tv_nsec / 1000000);

	if (now_ms > tm->start + tm->interval_ms) {
		tm->start = now_ms;
		return 1;
	}

	return 0;
}

void bare_timer_adjust_all(struct pp_instance *ppi, int32_t diff)
{
  /*	int i;

	for (i = 0; i < PP_TIMER_ARRAY_SIZE; i++) {
		ppi->timers[i]->start += diff;
	}
  */
}

int pp_timer_init(struct pp_instance *ppi)
	__attribute__((alias("bare_timer_init")));

int pp_timer_start(uint32_t interval_ms, struct pp_timer *tm)
	__attribute__((alias("bare_timer_start")));

int pp_timer_stop(struct pp_timer *tm)
	__attribute__((alias("bare_timer_stop")));

int pp_timer_expired(struct pp_timer *tm)
	__attribute__((alias("bare_timer_expired")));

void pp_timer_adjust_all(struct pp_instance *ppi, int32_t diff)
	__attribute__((alias("bare_timer_adjust_all")));
