#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "bare-linux.h"
#include <unistd.h>

static struct pp_timer bare_timers[PP_TIMER_ARRAY_SIZE];

int bare_timer_init(struct pp_instance *ppi)
{
	uint32_t i;

	for(i =0; i<PP_TIMER_ARRAY_SIZE; i++)
		ppi->timers[i] = &bare_timers[i];

	return 0;
}

int bare_timer_start(uint32_t interval, struct pp_timer *tm)
{
	tm->start = (uint32_t)sys_time(0);
	tm->interval = interval;
	return 0;
}

int bare_timer_stop(struct pp_timer *tm)
{
	tm->interval = 0;
	tm->start = 0;

	return 0;
}

int bare_timer_expired(struct pp_timer *tm)
{
	uint32_t now;

	if (tm->start == 0) {
		PP_PRINTF("%p Warning: bare_timer_expired: timer not started\n",tm);
		return 0;
	}

	now = (uint32_t)sys_time(0);

	if (tm->start + tm->interval <= (uint32_t)now) {
		tm->start = (uint32_t)now;
		return 1;
	}

	return 0;
}

void bare_timer_adjust_all(struct pp_instance *ppi, int32_t diff)
{
	int i;

	for (i = 0; i < PP_TIMER_ARRAY_SIZE; i++) {
		ppi->timers[i]->start += diff;
	}
}

int pp_timer_init(struct pp_instance *ppi)
	__attribute__((alias("bare_timer_init")));

int pp_timer_start(uint32_t interval, struct pp_timer *tm)
	__attribute__((alias("bare_timer_start")));

int pp_timer_stop(struct pp_timer *tm)
	__attribute__((alias("bare_timer_stop")));

int pp_timer_expired(struct pp_timer *tm)
	__attribute__((alias("bare_timer_expired")));

void pp_timer_adjust_all(struct pp_instance *ppi, int32_t diff)
	__attribute__((alias("bare_timer_adjust_all")));
