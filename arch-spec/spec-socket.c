/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */
#include <pptp/pptp.h>
#include "spec.h"
#include "include/syscon.h"
#include "include/minic.h"
#include<pptp/diag.h>

int spec_errno;
Octet buffer_out[PP_PACKET_SIZE + 14]; // 14 is ppi->proto_ofst for ethernet mode

/* This function should init the minic and get the mac address */
int spec_open_ch(struct pp_instance *ppi)
{
	uint8_t fake_addr[] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50};

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
	int got;

	led ^= 1; /* blink one led at each rx event */
	gpio_out(GPIO_PIN_LED_LINK, led);
	got = minic_rx_frame(pkt, pkt + 14, len - 14, NULL);
	pp_printf("%s: got=%d\n", __FUNCTION__, got);
	return got;
}

int spec_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype,
		     int use_pdelay_addr)
{
	static int led;
	struct ethhdr hdr;

	if (OPTS(ppi)->gptp_mode)
		memcpy(hdr.h_dest, PP_PEER_MACADDRESS, ETH_ALEN);
	else
		memcpy(hdr.h_dest, PP_MCAST_MACADDRESS, ETH_ALEN);

	memcpy(hdr.h_source, NP(ppi)->ch[PP_NP_GEN].addr, ETH_ALEN);
	hdr.h_proto = ETH_P_1588;

	pp_printf("%s: len=%d\n", __FUNCTION__, len);

	led ^= 1; /* blink the other led at each tx event */
	gpio_out(GPIO_PIN_LED_STATUS, led);
	return minic_tx_frame((uint8_t*)&hdr, pkt, len, NULL);
}

int spec_net_init(struct pp_instance *ppi)
{
	uint32_t i;
	ppi->buf_out = buffer_out;
	ppi->buf_out = PROTO_PAYLOAD(ppi->buf_out);

	//UDP only for now
	pp_printf("spec_net_init UDP\n");
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
int pp_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype,
		   int use_pdelay_addr)
	__attribute__((alias("spec_send_packet")));
