/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/* Socket interface for bare Linux */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "bare-i386.h"

/* 14 is ppi->proto_ofst for ethernet mode */
Octet buffer_out[PP_PACKET_SIZE + 14];

/* FIXME: which socket we receive and send with? */
int bare_recv_packet(struct pp_instance *ppi, void *pkt, int len,
		     TimeInternal *t)
{
	if (t)
		pp_get_tstamp(t);

	return sys_recv(NP(ppi)->ch[PP_NP_GEN].fd,
			pkt - NP(ppi)->proto_ofst, len, 0);
}

int bare_send_packet(struct pp_instance *ppi, void *pkt, int len,
		     TimeInternal *t, int chtype, int use_pdelay_addr)
{
	struct bare_ethhdr *hdr;
	hdr = PROTO_HDR(pkt);
	hdr->h_proto = htons(ETH_P_1588);

	if (OPTS(ppi)->gptp_mode)
		memcpy(hdr->h_dest, PP_PEER_MACADDRESS, 6);
	else
		memcpy(hdr->h_dest, PP_MCAST_MACADDRESS, 6);

	/* raw socket implementation always uses gen socket */
	memcpy(hdr->h_source, NP(ppi)->ch[PP_NP_GEN].addr, 6);

	if (t)
		pp_get_tstamp(t);

	return sys_send(NP(ppi)->ch[chtype].fd, hdr,
					len + NP(ppi)->proto_ofst, 0);
}

int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len, TimeInternal *t)
	__attribute__((alias("bare_recv_packet")));
int pp_send_packet(struct pp_instance *ppi, void *pkt, int len,
		   TimeInternal *t, int chtype, int use_pdelay_addr)
	__attribute__((alias("bare_send_packet")));

#define SHUT_RD		0
#define SHUT_WR		1
#define SHUT_RDWR	2

#define PF_PACKET 17
#define SOCK_RAW 3

/* To open a channel we must bind to an interface and so on */
int bare_open_ch(struct pp_instance *ppi, char *ifname)
{
	int sock = -1;
	int temp, iindex;
	struct bare_ifreq ifr;
	struct bare_sockaddr_ll addr_ll;
	struct bare_packet_mreq pmr;

	if (OPTS(ppi)->ethernet_mode) {
		/* open socket */
		sock = sys_socket(PF_PACKET, SOCK_RAW, ETH_P_1588);
		if (sock < 0) {
			pp_diag_error(ppi, bare_errno);
			pp_diag_fatal(ppi, "socket()", "");
			sys_close(sock);
			return -1;
		}

		/* hw interface information */
		memset(&ifr, 0, sizeof(ifr));
		strcpy(ifr.ifr_ifrn.ifrn_name, ifname);
		if (sys_ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
			pp_diag_error(ppi, bare_errno);
			pp_diag_fatal(ppi, "ioctl(GIFINDEX)", "");
			sys_close(sock);
			return -1;
		}

		iindex = ifr.ifr_ifru.index;
		if (sys_ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
			pp_diag_error(ppi, bare_errno);
			pp_diag_fatal(ppi, "ioctl(GIFHWADDR)", "");
			sys_close(sock);
			return -1;
		}

		memcpy(NP(ppi)->ch[PP_NP_GEN].addr,
					ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
		memcpy(NP(ppi)->ch[PP_NP_EVT].addr,
					ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);

		/* bind */
		memset(&addr_ll, 0, sizeof(addr_ll));
		addr_ll.sll_family = PF_PACKET;
		addr_ll.sll_protocol = htons(ETH_P_1588);
		addr_ll.sll_ifindex = iindex;
		if (sys_bind(sock, (struct bare_sockaddr *)&addr_ll,
							sizeof(addr_ll)) < 0) {
			pp_diag_error(ppi, bare_errno);
			pp_diag_fatal(ppi, "bind", "");
			sys_close(sock);
			return -1;
		}

		/* accept the multicast address for raw-ethernet ptp */
		memset(&pmr, 0, sizeof(pmr));
		pmr.mr_ifindex = iindex;
		pmr.mr_type = PACKET_MR_MULTICAST;
		pmr.mr_alen = ETH_ALEN;
		memcpy(pmr.mr_address, PP_MCAST_MACADDRESS, ETH_ALEN);
		sys_setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
			       &pmr, sizeof(pmr)); /* lazily ignore errors */

		/* also the PEER multicast address */
		memcpy(pmr.mr_address, PP_PEER_MACADDRESS, ETH_ALEN);
		sys_setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
			       &pmr, sizeof(pmr)); /* lazily ignore errors */

		NP(ppi)->ch[PP_NP_GEN].fd = sock;
		NP(ppi)->ch[PP_NP_EVT].fd = sock;

		/* make timestamps available through recvmsg() -- FIXME: hw? */
		sys_setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP,
			   &temp, sizeof(int));

		return 0;
	}
	return -1;
}

int bare_net_init(struct pp_instance *ppi)
{
	ppi->buf_out = buffer_out;
	ppi->buf_out = PROTO_PAYLOAD(ppi->buf_out);

	if (OPTS(ppi)->ethernet_mode) {
		PP_PRINTF("bare_net_init IEEE 802.3\n");

		/* raw sockets implementation always use gen socket */
		return bare_open_ch(ppi, OPTS(ppi)->iface_name);
	}

	/* else: UDP */
	PP_PRINTF("bare_net_init UDP\n");

	return 0;
}
int pp_net_init(struct pp_instance *ppi)
	__attribute__((alias("bare_net_init")));

int bare_net_shutdown(struct pp_instance *ppi)
{
	return sys_shutdown(NP(ppi)->ch[PP_NP_GEN].fd, SHUT_RDWR);
}
int pp_net_shutdown(struct pp_instance *ppi)
	__attribute__((alias("bare_net_shutdown")));
