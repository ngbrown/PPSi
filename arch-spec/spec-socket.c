/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */
#include <ppsi/ppsi.h>
#include "spec.h"
#include "include/syscon.h"
#include "include/minic.h"
#include <ppsi/diag.h>
#include <pps_gen.h>
#include "dev/softpll_ng.h"

int spec_errno;
Octet buffer_out[PP_PACKET_SIZE + 14]; // 14 is ppi->proto_ofst for ethernet mode

struct spec_socket_data_t
{
	uint32_t phase_transition;
	uint32_t dmtd_phase;
};

static struct spec_socket_data_t spec_socket_data;

/* This function should init the minic and get the mac address */
int spec_open_ch(struct pp_instance *ppi)
{
#ifdef PPSI_SLAVE
	uint8_t fake_addr[] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50};
#else
	uint8_t fake_addr[] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
#endif
	struct spec_socket_data_t *s;
	s = (struct spec_socket_data_t *)NP(ppi)->ch[PP_NP_EVT].arch_data;
	hexp_port_state_t pstate;

	ep_init(fake_addr);
	ep_enable(1, 1);
	minic_init();
	pps_gen_init();

	memcpy(NP(ppi)->ch[PP_NP_GEN].addr, fake_addr, 6);
	memcpy(NP(ppi)->ch[PP_NP_EVT].addr, fake_addr, 6);

	NP(ppi)->ch[PP_NP_EVT].arch_data = &spec_socket_data;

	if (halexp_get_port_state(&pstate, OPTS(ppi)->iface_name) < 0)
		return -1;

	s->phase_transition = pstate.t2_phase_transition;
	s->dmtd_phase = pstate.phase_val;

	return 0;
}

static inline int inside_range(int min, int max, int x)
{
  if(min < max)
    return (x>=min && x<=max);
  else
    return (x<=max || x>=min);
}

static void spec_linearize_rx_timestamp(TimeInternal *ts, int32_t dmtd_phase,
	int cntr_ahead, int transition_point, int clock_period)
{
	int trip_lo, trip_hi;
	int phase;

	// "phase" transition: DMTD output value (in picoseconds)
	// at which the transition of rising edge
	// TS counter will appear
	ts->raw_phase = dmtd_phase;

	phase = clock_period -1 -dmtd_phase;

	// calculate the range within which falling edge timestamp is stable
	// (no possible transitions)
	trip_lo = transition_point - clock_period / 4;
	if(trip_lo < 0) trip_lo += clock_period;

	trip_hi = transition_point + clock_period / 4;
	if(trip_hi >= clock_period) trip_hi -= clock_period;

	if (inside_range(trip_lo, trip_hi, phase)) {
		// We are within +- 25% range of transition area of
		// rising counter. Take the falling edge counter value as the
		// "reliable" one. cntr_ahead will be 1 when the rising edge
		//counter is 1 tick ahead of the falling edge counter

		ts->nanoseconds -= cntr_ahead ? (clock_period / 1000) : 0;

		// check if the phase is before the counter transition value
		// and eventually increase the counter by 1 to simulate a
		// timestamp transition exactly at s->phase_transition
		//DMTD phase value
		if(inside_range(trip_lo, transition_point, phase))
			ts->nanoseconds += clock_period / 1000;

	}

	ts->phase = phase - transition_point - 1;
	if(ts->phase  < 0) ts->phase += clock_period;
	ts->phase = clock_period - 1 - ts->phase;
}

/* To receive and send packets, we call the minic low-level stuff */
int spec_recv_packet(struct pp_instance *ppi, void *pkt, int len,
		     TimeInternal *t)
{
	static int led;
	struct hw_timestamp hwts;
	int got;
	struct spec_socket_data_t *s;

	s = (struct spec_socket_data_t *)NP(ppi)->ch[PP_NP_EVT].arch_data;

	led ^= 1; /* blink one led at each rx event */
	gpio_out(GPIO_PIN_LED_LINK, led);
	got = minic_rx_frame(pkt, pkt+ETH_HEADER_SIZE, len, &hwts);

	if (t) {
		t->seconds = hwts.sec;
		t->nanoseconds = hwts.nsec;
		t->phase = 0;
		t->correct = hwts.valid;

		t->raw_nsec = hwts.nsec;
		t->raw_ahead = hwts.ahead;
		spll_read_ptracker(0, &t->raw_phase, NULL);

		spec_linearize_rx_timestamp(t, t->raw_phase, hwts.ahead,
			s->phase_transition, REF_CLOCK_PERIOD_PS);
	}
	return got;
}

int spec_send_packet(struct pp_instance *ppi, void *pkt, int len,
			  TimeInternal *t, int chtype, int use_pdelay_addr)
{
	static int led;
	struct spec_ethhdr hdr;
	struct hw_timestamp hwts;
	int got;

	if (OPTS(ppi)->gptp_mode)
		memcpy(hdr.h_dest, PP_PEER_MACADDRESS, ETH_ALEN);
	else
		memcpy(hdr.h_dest, PP_MCAST_MACADDRESS, ETH_ALEN);

	memcpy(hdr.h_source, NP(ppi)->ch[PP_NP_GEN].addr, ETH_ALEN);
	hdr.h_proto = ETH_P_1588;

	led ^= 1; /* blink the other led at each tx event */
	gpio_out(GPIO_PIN_LED_STATUS, led);
	got = minic_tx_frame((uint8_t*)&hdr, pkt, len+ETH_HEADER_SIZE, &hwts);

	if (t) {
		t->seconds = hwts.sec;
		t->nanoseconds = hwts.nsec;
		t->phase = 0;
		t->correct = hwts.valid;

		PP_VPRINTF("%s: got=%d, sec=%d, nsec=%d\n", __FUNCTION__, got,
			   t->seconds, t->nanoseconds);
	}

	return got;
}

int spec_net_init(struct pp_instance *ppi)
{
	uint32_t i;
	ppi->buf_out = buffer_out;
	ppi->buf_out = PROTO_PAYLOAD(ppi->buf_out);

	ppi->buf_out+= 4 - (((int)ppi->buf_out) % 4);
	/* FIXME Alignment */
	pp_printf("spec_net_init ETH %x\n",ppi->buf_out);
	spec_open_ch(ppi);

	return 0;

}

int spec_net_shutdown(struct pp_instance *ppi)
{
	//GGDD
	return 0;
}

int pp_net_init(struct pp_instance *ppi)
	__attribute__((alias("spec_net_init")));
int pp_net_shutdown(struct pp_instance *ppi)
	__attribute__((alias("spec_net_shutdown")));
int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len, TimeInternal *t)
	__attribute__((alias("spec_recv_packet")));
int pp_send_packet(struct pp_instance *ppi, void *pkt, int len, TimeInternal *t,
				   int chtype, int use_pdelay_addr)
	__attribute__((alias("spec_send_packet")));
