#include "../spec.h"

//#include <inttypes.h>

#include <hw/pps_gen_regs.h>

#define PPS_PULSE_WIDTH 100000

static volatile struct PPSG_WB *PPSG = (volatile struct PPS_GEN_WB *) BASE_PPSGEN;

static inline uint32_t ppsg_readl(uint32_t reg)
{
	return *(volatile uint32_t *)(BASE_PPSGEN + reg);
}

void pps_gen_init()
{
	uint32_t cr;

	cr = PPSG_CR_CNT_EN | PPSG_CR_PWIDTH_W(PPS_PULSE_WIDTH);

	PPSG->CR = cr;
	PPSG->ESCR = 0;

	PPSG->ADJ_UTCLO = 100;
	PPSG->ADJ_UTCHI = 0;
	PPSG->ADJ_NSEC = 0;

#ifdef PPSI_SLAVE
	/* Random initial time:q for slave */
	PPSG->ADJ_UTCLO = 123456;
	PPSG->ADJ_NSEC = 987654;
#endif

	PPSG->CR = cr | PPSG_CR_CNT_SET;
	PPSG->CR = cr;
}

void pps_gen_set(int32_t sec, int32_t nsec)
{
	TRACE_DEV("ADJ: sec %d nsec %d\n", sec, nsec);
	PPSG->ADJ_UTCLO = sec;
	PPSG->ADJ_UTCHI = 0;

	PPSG->ADJ_NSEC = nsec;

	PPSG->CR =  PPSG_CR_CNT_EN | PPSG_CR_PWIDTH_W(PPS_PULSE_WIDTH) |
				PPSG_CR_CNT_SET;
}

void pps_gen_adjust_nsec(int32_t how_much)
{
	TRACE_DEV("ADJ: nsec %d nanoseconds\n", how_much);
#if 1
	PPSG->ADJ_UTCLO = 0;
	PPSG->ADJ_UTCHI = 0;

	PPSG->ADJ_NSEC = ( how_much / 8 );

	PPSG->CR =  PPSG_CR_CNT_EN | PPSG_CR_PWIDTH_W(PPS_PULSE_WIDTH) | PPSG_CR_CNT_ADJ;
#endif

}

void pps_gen_adjust_utc(int32_t how_much)
{

#if 1
	TRACE_DEV("ADJ: utc %d seconds\n", how_much);
	PPSG->ADJ_UTCLO = how_much;
	PPSG->ADJ_UTCHI = 0;
	PPSG->ADJ_NSEC = 0;
	PPSG->CR = PPSG_CR_CNT_EN | PPSG_CR_PWIDTH_W(PPS_PULSE_WIDTH) | PPSG_CR_CNT_ADJ;
#endif

}

int pps_gen_busy()
{
	return PPSG->CR & PPSG_CR_CNT_ADJ ? 0 : 1;
}

void pps_gen_get_time(uint32_t *utc, uint32_t *cntr_nsec)
{
	uint32_t nsec;
	uint32_t utc_lo1, utc_lo2;

	utc_lo1 = PPSG->CNTR_UTCLO;
	nsec = PPSG->CNTR_NSEC & 0xfffffff;
	utc_lo2 = PPSG->CNTR_UTCLO;
	if (utc_lo2 != utc_lo1)
		nsec = PPSG->CNTR_NSEC & 0xfffffff;

	if(utc) *utc = utc_lo2;
	if(cntr_nsec) *cntr_nsec = nsec;
}


void pps_gen_enable_output(int enable)
{
	if(enable)
		PPSG->ESCR = PPSG_ESCR_PPS_VALID  | PPSG_ESCR_TM_VALID;
	else
		PPSG->ESCR = 0;
}

