/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 * Aurelio Colosimo for CERN, 2012 -- GNU LGPL v2.1 or later
 */
#ifndef __SPEC_H
#define __SPEC_H

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include <hw/memlayout.h>
/*
 * These are the functions provided by the various bare files
 */
extern int spec_open_ch(struct pp_instance *ppi);

extern int spec_recv_packet(struct pp_instance *ppi, void *pkt, int len,
			    TimeInternal *t);
extern int spec_send_packet(struct pp_instance *ppi, void *pkt, int len,
			TimeInternal *t, int chtype, int use_pdelay_addr);

extern void spec_main_loop(struct pp_instance *ppi);
extern void _irq_entry(void); /* unused, to make crt0.S happy */
extern int main(void); /* alias to ppsi_main, so crt0.S is happy */

/* FIXME halexp_port_state, to be renamed and moved somewhere else */

typedef struct {
  /* When non-zero: port state is valid */
  int valid;

  /* WR-PTP role of the port (Master, Slave, etc.) */
  int mode;

  /* TX and RX delays (combined, big Deltas from the link model in the spec) */
  uint32_t delta_tx;
  uint32_t delta_rx;

  /* DDMTD raw phase value in picoseconds */
  uint32_t phase_val;

  /* When non-zero: phase_val contains a valid phase readout */
  int phase_val_valid;

  /* When non-zero: link is up */
  int up;

  /* When non-zero: TX path is calibrated (delta_tx contains valid value) */
  int tx_calibrated;

  /* When non-zero: RX path is calibrated (delta_rx contains valid value) */
  int rx_calibrated;
  int tx_tstamp_counter;
  int rx_tstamp_counter;
  int is_locked;
  int lock_priority;

  // timestamp linearization paramaters

  uint32_t phase_setpoint; // DMPLL phase setpoint (picoseconds)

  uint32_t clock_period; // reference lock period in picoseconds
  uint32_t t2_phase_transition; // approximate DMTD phase value (on slave port) at which RX timestamp (T2) counter transistion occurs (picoseconds)

  uint32_t t4_phase_transition; // approximate phase value (on master port) at which RX timestamp (T4) counter transistion occurs (picoseconds)

  uint8_t hw_addr[6];
  int hw_index;
  int32_t fiber_fix_alpha;
} hexp_port_state_t;

extern int halexp_get_port_state(hexp_port_state_t *state,
				 const char *port_name);

/* End halexp_port_state */

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

#define CPU_CLOCK	62500000ULL

#define UART_BAUDRATE	115200ULL /* not a real UART */

#define GPIO_PIN_LED_LINK	0
#define GPIO_PIN_LED_STATUS	1
#define GPIO_PIN_SCL_OUT	2
#define GPIO_PIN_SDA_OUT	3
#define GPIO_PIN_SDA_IN		4
#define GPIO_PIN_BTN1		5
#define GPIO_PIN_BTN2		6

/* hacks to use code imported from other places (wrpc-software) */
#define TRACE_DEV pp_printf
#define mprintf pp_printf

struct hw_timestamp {
	uint8_t valid;
	int ahead;
	uint64_t sec;
	uint32_t nsec;
	uint32_t phase;
};

#define DMTD_AVG_SAMPLES 256
#define DMTD_MAX_PHASE 16384

#ifndef NULL
#define NULL 0
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((int) &((TYPE *)0)->MEMBER)
#endif

#define REF_CLOCK_PERIOD_PS 8000
#define REF_CLOCK_FREQ_HZ 125000000

#endif
