#include "syscon.h"

struct s_i2c_if i2c_if[2] = { {SYSC_GPSR_FMC_SCL, SYSC_GPSR_FMC_SDA},
                              {SYSC_GPSR_SFP_SCL, SYSC_GPSR_SFP_SDA} };

/****************************
 *        TIMER
 ***************************/
void timer_init(uint32_t enable)
{
	if(enable)
		syscon->TCR |= SYSC_TCR_ENABLE;
	else
		syscon->TCR &= ~SYSC_TCR_ENABLE;
}

uint32_t timer_get_tics()
{
	return syscon->TVR;
}

void spec_udelay(int usecs)
{
	uint32_t start, end;

	timer_init(1);

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
