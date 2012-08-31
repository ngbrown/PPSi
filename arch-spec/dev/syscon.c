#include "syscon.h"

struct s_i2c_if i2c_if[2] = { {SYSC_GPSR_FMC_SCL, SYSC_GPSR_FMC_SDA},
                              {SYSC_GPSR_SFP_SCL, SYSC_GPSR_SFP_SDA} };

volatile struct SYSCON_WB *syscon;

/****************************
 *        TIMER
 ***************************/
void timer_init(uint32_t enable)
{
	syscon = (volatile struct SYSCON_WB *) BASE_SYSCON;
	if(enable)
		syscon->TCR |= SYSC_TCR_ENABLE;
	else
		syscon->TCR &= ~SYSC_TCR_ENABLE;
}

uint32_t timer_get_tics()
{
	return syscon->TVR;
}

void timer_delay(uint32_t usecs)
{
	uint32_t start, end;

	timer_init(1);

	start = timer_get_tics();
	/* It looks like the counter counts millisecs */
	end = start + (usecs + 500) / 1000;

	while ((signed)(end - timer_get_tics()) > 0)
		;
}

/*
void timer_delay(uint32_t how_long)
{
  uint32_t t_start;

//  timer_init(1);
  do
  {
    t_start = timer_get_tics();
  } while(t_start > UINT32_MAX - how_long); //in case of overflow

  while(t_start + how_long > timer_get_tics());
}*/

/* return a monotonic seconds count from the counter above; horrible code */
int spec_time(void)
{
	static uint32_t prev, secs;
	static int rest; /* millisecs */
	uint32_t tics = timer_get_tics();

	if (!prev) {
		prev = tics;
		secs = 1; /* Start from a small number! */
		return secs;
	}
	rest += tics - prev;
	secs += rest / 1000;
	rest %= 1000;
	prev = tics;
	return secs;
}
