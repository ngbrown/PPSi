/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

/* Socket interface for bare Linux */
#include <ppsi/ppsi.h>
#include "ptpdump.h"
#include "bare-linux.h"

/* FIXME: which socket we receive and send with? */
static int bare_net_recv(struct pp_instance *ppi, void *pkt, int len,
			    TimeInternal *t)
{
	int ret;
	if (t)
		ppi->t_ops->get(ppi, t);

	ret = sys_recv(NP(ppi)->ch[PP_NP_GEN].fd, pkt, len, 0);
	if (ret > 0 && pp_diag_allow(ppi, frames, 2))
		dump_1588pkt("recv: ", pkt, ret, t);
	return ret;
}

static int bare_net_send(struct pp_instance *ppi, void *pkt, int len,
			    TimeInternal *t, int chtype, int use_pdelay_addr)
{
	struct bare_ethhdr *hdr = pkt;
	int ret;

	hdr->h_proto = htons(ETH_P_1588);

	memcpy(hdr->h_dest, PP_MCAST_MACADDRESS, 6);

	/* raw socket implementation always uses gen socket */
	memcpy(hdr->h_source, NP(ppi)->ch[PP_NP_GEN].addr, 6);

	if (t)
		ppi->t_ops->get(ppi, t);

	ret = sys_send(NP(ppi)->ch[chtype].fd, pkt, len, 0);
	if (ret > 0 && pp_diag_allow(ppi, frames, 2))
		dump_1588pkt("send: ", pkt, len, t);
	return ret;
}

#define SHUT_RD		0
#define SHUT_WR		1
#define SHUT_RDWR	2

#define PF_PACKET 17
#define SOCK_RAW 3

/* To open a channel we must bind to an interface and so on */
static int bare_open_ch(struct pp_instance *ppi, char *ifname)
{
	int sock = -1;
	int temp, iindex;
	struct bare_ifreq ifr;
	struct bare_sockaddr_ll addr_ll;
	struct bare_packet_mreq pmr;
	char *context;

	if (ppi->proto == PPSI_PROTO_RAW) {
		/* open socket */
		context = "socket()";
		sock = sys_socket(PF_PACKET, SOCK_RAW, ETH_P_1588);
		if (sock < 0)
			goto err_out;

		/* hw interface information */
		memset(&ifr, 0, sizeof(ifr));
		strcpy(ifr.ifr_ifrn.ifrn_name, ifname);
		context = "ioctl(SIOCGIFINDEX)";
		if (sys_ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
			goto err_out;

		iindex = ifr.ifr_ifru.index;
		context = "ioctl(SIOCGIFHWADDR)";
		if (sys_ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
			goto err_out;

		memcpy(NP(ppi)->ch[PP_NP_GEN].addr,
					ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
		memcpy(NP(ppi)->ch[PP_NP_EVT].addr,
					ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);

		/* bind */
		memset(&addr_ll, 0, sizeof(addr_ll));
		addr_ll.sll_family = PF_PACKET;
		addr_ll.sll_protocol = htons(ETH_P_1588);
		addr_ll.sll_ifindex = iindex;
		context = "bind()";
		if (sys_bind(sock, (struct bare_sockaddr *)&addr_ll,
							sizeof(addr_ll)) < 0)
			goto err_out;

		/* accept the multicast address for raw-ethernet ptp */
		memset(&pmr, 0, sizeof(pmr));
		pmr.mr_ifindex = iindex;
		pmr.mr_type = PACKET_MR_MULTICAST;
		pmr.mr_alen = ETH_ALEN;
		memcpy(pmr.mr_address, PP_MCAST_MACADDRESS, ETH_ALEN);
		sys_setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
			   &pmr, sizeof(pmr)); /* lazily ignore errors */

		NP(ppi)->ch[PP_NP_GEN].fd = sock;
		NP(ppi)->ch[PP_NP_EVT].fd = sock;

		/* make timestamps available through recvmsg() -- FIXME: hw? */
		sys_setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP,
			   &temp, sizeof(int));

		return 0;
	}
	pp_printf("%s: can't init UDP channels yet\n", __func__);
	return -1;

err_out:
	pp_printf("%s: %s: fatal error, errno = %i\n", __func__, context,
		  bare_errno);
	if (sock >= 0)
		sys_close(sock);
	return -1;
}

static int bare_net_exit(struct pp_instance *ppi)
{
	return sys_shutdown(NP(ppi)->ch[PP_NP_GEN].fd, SHUT_RDWR);
}

/* This function must be able to be called twice, and clean-up internally */
static int bare_net_init(struct pp_instance *ppi)
{
	/* Here, socket may not be 0 (do we have stdin even if bare) */
	if (NP(ppi)->ch[PP_NP_GEN].fd)
		bare_net_exit(ppi);

	/* The buffer is inside ppi, but we need to set pointers and align */
	pp_prepare_pointers(ppi);

	if (ppi->proto == PPSI_PROTO_RAW) {
		pp_diag(ppi, frames, 1, "bare_net_init IEEE 802.3\n");

		/* raw sockets implementation always use gen socket */
		return bare_open_ch(ppi, ppi->iface_name);
	}

	/* else: UDP -- not supported */
	pp_diag(ppi, frames, 1, "bare_net_init UDP\n");
	return -1;
}


struct pp_network_operations bare_net_ops = {
	.init = bare_net_init,
	.exit = bare_net_exit,
	.recv = bare_net_recv,
	.send = bare_net_send,
};
