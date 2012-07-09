#include <ppsi/ppsi.h>
#include <pps_gen.h>

static struct pp_timer spec_timers[PP_TIMER_ARRAY_SIZE];

int spec_timer_init(struct pp_instance *ppi)
{
	uint32_t i;

	for(i =0; i<PP_TIMER_ARRAY_SIZE; i++)
		ppi->timers[i] = &spec_timers[i];

	return 0;
}

int spec_timer_start(uint32_t interval, struct pp_timer *tm)
{
	uint32_t nsec;

	tm->start = spec_time();
	tm->interval = interval;
	return 0;
}

int spec_timer_stop(struct pp_timer *tm)
{
	tm->interval = 0;
	tm->start = 0;

	return 0;
}

int spec_timer_expired(struct pp_timer *tm)
{
	uint32_t now, nsec;

	if (tm->start == 0) {
		PP_PRINTF("%p Warning: posix_timer_expired: timer not started\n",tm);
		return 0;
	}

	now = spec_time();

	if (tm->start + tm->interval <= now) {
		tm->start = now;
		return 1;
	}

	return 0;
}

void spec_timer_adjust_all(struct pp_instance *ppi, int32_t diff)
{
	/* No need for timer adjust in SPEC: it uses a sort of monotonic
	 * tstamp for timers
	*/
}



int pp_timer_init(struct pp_instance *ppi)
	__attribute__((alias("spec_timer_init")));

int pp_timer_start(uint32_t interval, struct pp_timer *tm)
	__attribute__((alias("spec_timer_start")));

int pp_timer_stop(struct pp_timer *tm)
	__attribute__((alias("spec_timer_stop")));

int pp_timer_expired(struct pp_timer *tm)
	__attribute__((alias("spec_timer_expired")));

void pp_timer_adjust_all(struct pp_instance *ppi, int32_t diff)
	__attribute__((alias("spec_timer_adjust_all")));
