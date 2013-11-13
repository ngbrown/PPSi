/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */
#include <ppsi/ppsi.h>
#include "ptpdump.h"

#include <ptpd_netif.h> /* wrpc-sw */


/* This function should init the minic and get the mac address */
static int wrpc_open_ch(struct pp_instance *ppi)
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
static int wrpc_net_recv(struct pp_instance *ppi, void *pkt, int len,
			 TimeInternal *t)
{
	int got;
	wr_socket_t *sock;
	wr_timestamp_t wr_ts;
	wr_sockaddr_t addr;
	sock = NP(ppi)->ch[PP_NP_EVT].custom;
	got = ptpd_netif_recvfrom(sock, &addr, pkt, len, &wr_ts);

	if (t) {
		t->seconds = wr_ts.sec;
		t->nanoseconds = wr_ts.nsec;
		t->phase = wr_ts.phase;
		t->correct = wr_ts.correct;
#if 0 /* I disabled the fields, for space: they were only used here */
		t->raw_nsec = wr_ts.raw_nsec;
		t->raw_phase = wr_ts.raw_phase;
#endif
		t->raw_ahead = wr_ts.raw_ahead;
	}

/* wrpc-sw may pass this in USER_CFLAGS, to remove footprint */
#ifndef CONFIG_NO_PTPDUMP
	/* The header is separate, so dump payload only */
	if (got > 0 && pp_diag_allow(ppi, frames, 2))
		dump_payloadpkt("recv: ", pkt, got, t);
#endif

	return got;
}

static int wrpc_net_send(struct pp_instance *ppi, void *pkt, int len,
			 TimeInternal *t, int chtype, int use_pdelay_addr)
{
	int snt;
	wr_socket_t *sock;
	wr_timestamp_t wr_ts;
	wr_sockaddr_t addr;
	sock = NP(ppi)->ch[PP_NP_EVT].custom;

	addr.ethertype = ETH_P_1588;
	memcpy(&addr.mac, PP_MCAST_MACADDRESS, sizeof(mac_addr_t));

	snt = ptpd_netif_sendto(sock, &addr, pkt, len, &wr_ts);

	if (t) {
		t->seconds = wr_ts.sec;
		t->nanoseconds = wr_ts.nsec;
		t->phase = 0;
		t->correct = wr_ts.correct;

		pp_diag(ppi, frames, 2, "%s: snt=%d, sec=%d, nsec=%d\n",
				__func__, snt, t->seconds, t->nanoseconds);
	}
/* wrpc-sw may pass this in USER_CFLAGS, to remove footprint */
#ifndef CONFIG_NO_PTPDUMP
	/* The header is separate, so dump payload only */
	if (snt >0 && pp_diag_allow(ppi, frames, 2))
		dump_payloadpkt("send: ", pkt, len, t);
#endif

	return snt;
}

static int wrpc_net_exit(struct pp_instance *ppi)
{
	ptpd_netif_close_socket(NP(ppi)->ch[PP_NP_EVT].custom);
	return 0;
}

/* This function must be able to be called twice, and clean-up internally */
static int wrpc_net_init(struct pp_instance *ppi)
{
	if (NP(ppi)->ch[PP_NP_EVT].custom)
		wrpc_net_exit(ppi);
	pp_prepare_pointers(ppi);
	wrpc_open_ch(ppi);

	return 0;

}

struct pp_network_operations wrpc_net_ops = {
	.init = wrpc_net_init,
	.exit = wrpc_net_exit,
	.recv = wrpc_net_recv,
	.send = wrpc_net_send,
};
