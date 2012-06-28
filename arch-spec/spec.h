/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */
#ifndef __SPEC_H
#define __SPEC_H

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
/*
 * These are the functions provided by the various bare files
 */
extern int spec_open_ch(struct pp_instance *ppi);

extern int spec_recv_packet(struct pp_instance *ppi, void *pkt, int len,
			    TimeInternal *t);
extern int spec_send_packet(struct pp_instance *ppi, void *pkt, int len,
			int chtype, int use_pdelay_addr);

extern void spec_main_loop(struct pp_instance *ppi);
extern void _irq_entry(void); /* unused, to make crt0.S happy */
extern int main(void); /* alias to ppsi_main, so crt0.S is happy */

/* basics */
extern void *memset(void *s, int c, int count);
extern char *strcpy(char *dest, const char *src);
extern void *memcpy(void *dest, const void *src, int count);

/* syscall-lookalike */
extern int spec_time(void);
extern void spec_udelay(int usecs);
extern int spec_errno;

/* Dev stuff */
extern void spec_uart_init(void);
extern void spec_putc(int c);
extern void spec_puts(const char *s);
extern int spec_testc(void);
extern int spec_getc(void);

extern void minic_init(void);
extern int minic_poll_rx(void);
struct hw_timestamp;
extern int minic_rx_frame(uint8_t *hdr, uint8_t *payload, uint32_t buf_size,
			  struct hw_timestamp *hwts /* unused */);
extern int minic_tx_frame(uint8_t *hdr, uint8_t *payload, uint32_t size,
			  struct hw_timestamp *hwts /* unused */);


extern void ep_init(uint8_t mac_addr[]);
extern void get_mac_addr(uint8_t dev_addr[]);
extern int ep_enable(int enabled, int autoneg);
extern int ep_link_up();
extern int ep_get_deltas(uint32_t *delta_tx, uint32_t *delta_rx);
extern int ep_get_psval(int32_t *psval);
extern int ep_cal_pattern_enable();
extern int ep_cal_pattern_disable();

static inline void delay(int x)
{
	while(x--) asm volatile("nop");
}

/* other network lstuff, bah.... */

struct spec_ethhdr {
	unsigned char	h_dest[6];
	unsigned char	h_source[6];
	uint16_t	h_proto;
} __attribute__((packed));

/* Low-level details (from board.h in wr-core-tools) */

#define BASE_MINIC	  0x20000
#define BASE_EP		    0x20100
#define BASE_SOFTPLL	0x20200
#define BASE_PPSGEN	  0x20300
#define BASE_SYSCON   0x20400
#define BASE_UART	    0x20500
#define BASE_ONEWIRE  0x20600
//#define BASE_TIMER	  0x61000

#define CPU_CLOCK	62500000ULL

#define UART_BAUDRATE	115200ULL /* not a real UART */

#define GPIO_PIN_LED_LINK	0
#define GPIO_PIN_LED_STATUS	1
#define GPIO_PIN_SCL_OUT	2
#define GPIO_PIN_SDA_OUT	3
#define GPIO_PIN_SDA_IN		4
#define GPIO_PIN_BTN1		5
#define GPIO_PIN_BTN2		6

/* hacks to use code imported from other places (wr-core-software) */
#define TRACE_DEV pp_printf

struct hw_timestamp {
	int ahead;
	uint32_t utc;
	uint32_t nsec;
	uint32_t phase;
};

#define DMTD_AVG_SAMPLES 256
#define DMTD_MAX_PHASE 16384

#define NULL 0

#endif
