
/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 * based on code by Tomasz Wlostowski
 */

#include <pproto/pproto.h>
#include "../spec.h"


static uint32_t timer_get_tics(void)
{
	return *(volatile uint32_t *)BASE_TIMER;
}

void spec_udelay(int usecs)
{
	uint32_t start, end;

	start = timer_get_tics();
	/* It looks like the counter counts millisecs */
	end = start + (usecs + 500) / 1000;

	while ((signed)(end - timer_get_tics()) > 0)
		;
}

/* return a seconds count from the counter above; horrible code */
int spec_time(void)
{
	static uint32_t prev, secs;
	static int rest; /* millisecs */
	uint32_t tics = timer_get_tics();

	if (!prev) {
		prev = tics;
		secs = 1293836400; /* jan 1st 2011 or a random number */
		return secs;
	}
	rest += tics - prev;
	secs += rest / 1000;
	rest %= 1000;
	prev = tics;
	return secs;
}
