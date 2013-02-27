/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>
#include <uart.h>
#include <pps_gen.h>
#include "wrpc.h"

void pp_puts(const char *s)
{
	uart_write_string(s);
}

void pp_get_tstamp(TimeInternal *t) //uint32_t *sptr)
{
	uint64_t sec;
	uint32_t nsec;
	shw_pps_gen_get_time(&sec, &nsec);
	t->seconds = (int32_t)sec;
	t->nanoseconds = (int32_t)nsec;
}

int32_t wrpc_set_tstamp(TimeInternal *t)
{
	shw_pps_gen_set_time(t->seconds, t->nanoseconds);

	return 0; /* SPEC uses a sort of monotonic tstamp for timers */
}

int wrpc_adj_freq(Integer32 adj)
{
	/* FIXME: this adjusts nanoseconds, not frequency */
	shw_pps_gen_adjust(PPSG_ADJUST_NSEC, adj);
	return -1; /* failure? */
}

int pp_adj_freq(Integer32 adj)
	__attribute__((alias("wrpc_adj_freq")));

int32_t pp_set_tstamp(TimeInternal *t)
	__attribute__((alias("wrpc_set_tstamp")));
