/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/* Socket interface for bare Linux */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "bare-linux.h"

/* 14 is ppi->proto_ofst for ethernet mode */
Octet buffer_out[PP_PACKET_SIZE + 14];

/* FIXME: which socket we receive and send with? */
int bare_recv_packet(struct pp_instance *ppi, void *pkt, int len,
		     TimeInternal *t)
{
	return sys_recv(NP(ppi)->ch[PP_NP_GEN].fd, pkt, len, 0);
}

int bare_send_packet(struct pp_instance *ppi, void *pkt, int len, 
		     TimeInternal *t, int chtype, int use_pdelay_addr)
{
	return sys_send(NP(ppi)->ch[chtype].fd, pkt, len, 0);
}

int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len, TimeInternal *t)
	__attribute__((alias("bare_recv_packet")));
int pp_send_packet(struct pp_instance *ppi, void *pkt, int len, 
		   TimeInternal *t, int chtype, int use_pdelay_addr)
	__attribute__((alias("bare_send_packet")));

#define SHUT_RD		0
#define SHUT_WR 	1
#define SHUT_RDWR 	2

#define PF_PACKET 17
#define SOCK_RAW 3

/* To open a channel we must bind to an interface and so on */
int bare_open_ch(struct pp_instance *ppi, char *ifname)
{
	int sock, iindex;
	struct bare_ifreq ifr;
	struct bare_sockaddr_ll addr;

	/* open socket */
	sock = sys_socket(PF_PACKET, SOCK_RAW, PP_ETHERTYPE);
	if (sock < 0) {
		pp_diag_error(ppi, bare_errno);
		pp_diag_fatal(ppi, "socket()", "");
	}

	/* hw interface information */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_ifrn.ifrn_name, ifname);
	if (sys_ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		pp_diag_error(ppi, bare_errno);
		pp_diag_fatal(ppi, "ioctl(GIFINDEX)", "");
	}

	iindex = ifr.ifr_ifru.index;
	if (sys_ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
		pp_diag_error(ppi, bare_errno);
		pp_diag_fatal(ppi, "ioctl(GIFHWADDR)", "");
	}
	memcpy(NP(ppi)->ch[PP_NP_GEN].addr,
	       ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
	memcpy(NP(ppi)->ch[PP_NP_EVT].addr,
	       ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);

	/* bind and setsockopt */
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = PF_PACKET;
	addr.sll_protocol = htons(PP_ETHERTYPE);
	addr.sll_ifindex = iindex;
	if (sys_bind(sock, (struct bare_sockaddr *)&addr, sizeof(addr)) < 0) {
		pp_diag_error(ppi, bare_errno);
		pp_diag_fatal(ppi, "bind", "");
	}

	NP(ppi)->ch[PP_NP_GEN].fd = sock;
	NP(ppi)->ch[PP_NP_EVT].fd = sock;
	return 0;
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
