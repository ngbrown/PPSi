/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */
#include <ppsi/ppsi.h>
#include "spec.h"
#include <syscon.h>
#include <pps_gen.h>
#include <minic.h>
#include <ppsi/diag.h>
#include <pps_gen.h>
#include <softpll_ng.h>
#include <ptpd_netif.h>

int spec_errno;
Octet buffer_out[PP_PACKET_SIZE + 14]; // 14 is ppi->proto_ofst for ethernet mode

/* This function should init the minic and get the mac address */
int spec_open_ch(struct pp_instance *ppi)
{
	wr_socket_t *sock;
	mac_addr_t mac;
	wr_sockaddr_t addr;

	addr.family = PTPD_SOCK_RAW_ETHERNET;
	addr.ethertype = ETH_P_1588;
	memcpy(addr.mac, PP_MCAST_MACADDRESS, sizeof(mac_addr_t));

	sock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0, &addr);
	if (!sock)
		return -1;

	ptpd_netif_get_hw_addr(sock, &mac);
	memcpy(NP(ppi)->ch[PP_NP_EVT].addr, &mac, sizeof(mac_addr_t));
	NP(ppi)->ch[PP_NP_EVT].custom = sock;
	memcpy(NP(ppi)->ch[PP_NP_GEN].addr, &mac, sizeof(mac_addr_t));
	NP(ppi)->ch[PP_NP_GEN].custom = sock;

	return 0;
}

/* To receive and send packets, we call the minic low-level stuff */
int spec_recv_packet(struct pp_instance *ppi, void *pkt, int len,
		     TimeInternal *t)
{
	int got;
	wr_socket_t *sock;
	wr_timestamp_t wr_ts;
	wr_sockaddr_t addr;
	sock = (wr_socket_t *)NP(ppi)->ch[PP_NP_EVT].custom;
	got = ptpd_netif_recvfrom(sock, &addr, pkt, len, &wr_ts);

	/* FIXME check mac of received packet? */

	if (t) {
		t->seconds = wr_ts.sec;
		t->nanoseconds = wr_ts.nsec;
		t->phase = wr_ts.phase;
		t->correct = wr_ts.correct;
		t->raw_nsec = wr_ts.raw_nsec;
		t->raw_ahead = wr_ts.raw_ahead;
		t->raw_phase = wr_ts.raw_phase;
	}
	return got;
}

int spec_send_packet(struct pp_instance *ppi, void *pkt, int len,
			  TimeInternal *t, int chtype, int use_pdelay_addr)
{
	int snt;
	wr_socket_t *sock;
	wr_timestamp_t wr_ts;
	wr_sockaddr_t addr;
	sock = (wr_socket_t *)NP(ppi)->ch[PP_NP_EVT].custom;

	if (pp_diag_verbosity > 1) {
		int j;
		pp_printf("sent: %i\n", len);
		for (j = 0; j < len; j++) {
			pp_printf("%02x ", ((char*)pkt)[j]);
			if( (j+1)%16==0 )
				pp_printf("\n");
		}
		pp_printf("\n");
	}

	addr.ethertype = ETH_P_1588;
	memcpy(&addr.mac, PP_MCAST_MACADDRESS, sizeof(mac_addr_t));

	snt = ptpd_netif_sendto(sock, &addr, pkt, len, &wr_ts);

	if (t) {
		t->seconds = wr_ts.sec;
		t->nanoseconds = wr_ts.nsec;
		t->phase = 0;
		t->correct = wr_ts.correct;

		PP_VPRINTF("%s: snt=%d, sec=%d, nsec=%d\n", __FUNCTION__, snt,
			   t->seconds, t->nanoseconds);
	}

	return snt;
}

int spec_net_init(struct pp_instance *ppi)
{
	ppi->buf_out = buffer_out;
	ppi->buf_out = PROTO_PAYLOAD(ppi->buf_out);

	ppi->buf_out+= 4 - (((int)ppi->buf_out) % 4);
	/* FIXME Alignment */
	pp_printf("spec_net_init ETH %x\n", (uint32_t)ppi->buf_out);
	spec_open_ch(ppi);

	return 0;

}

int spec_net_shutdown(struct pp_instance *ppi)
{
	ptpd_netif_close_socket(
		(wr_socket_t *)NP(ppi)->ch[PP_NP_EVT].custom);
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
