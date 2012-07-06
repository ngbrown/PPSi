/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */
#include <ppsi/ppsi.h>
#include "spec.h"
#include "include/syscon.h"
#include "include/minic.h"
#include<ppsi/diag.h>

int spec_errno;
Octet buffer_out[PP_PACKET_SIZE + 14]; // 14 is ppi->proto_ofst for ethernet mode

/* This function should init the minic and get the mac address */
int spec_open_ch(struct pp_instance *ppi)
{
#ifdef PPSI_SLAVE
	uint8_t fake_addr[] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50};
#else
	uint8_t fake_addr[] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
#endif

	ep_init(fake_addr);
	ep_enable(1, 0);
	minic_init();
	pps_gen_init();

	memcpy(NP(ppi)->ch[PP_NP_GEN].addr, fake_addr, 6);
	memcpy(NP(ppi)->ch[PP_NP_EVT].addr, fake_addr, 6);

	return 0;
}

/* To receive and send packets, we call the minic low-level stuff */
int spec_recv_packet(struct pp_instance *ppi, void *pkt, int len,
		     TimeInternal *t)
{
	static int led;
	struct hw_timestamp hwts;
	int got;

	led ^= 1; /* blink one led at each rx event */
	gpio_out(GPIO_PIN_LED_LINK, led);
	got = minic_rx_frame(pkt, pkt+ETH_HEADER_SIZE, len, &hwts);

	if (t) {
		//add phase value and linearize function for WR support
		t->seconds = hwts.utc;
		t->nanoseconds = hwts.nsec;

		PP_VPRINTF("%s: got=%d, sec=%d, nsec=%d\n", __FUNCTION__, got,
			   t->seconds, t->nanoseconds);
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
		/* add phase value and linearize function for WR support */
		t->seconds = hwts.utc;
		t->nanoseconds = hwts.nsec;

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
