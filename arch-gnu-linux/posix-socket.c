/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/* Socket interface for GNU/Linux (and most likely other posix systems */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <arpa/inet.h>


#include <pptp/pptp.h>
#include <pptp/diag.h>
#include "posix.h"

static int ch_check_stat = 0;

/* Receive and send is *not* so trivial */
int posix_recv_packet(struct pp_instance *ppi, void *pkt, int len,
	TimeInternal *t)
{
	struct pp_channel *ch1 = NULL, *ch2 = NULL;

	/* TODO: rewrite it in a better way! */
	switch (ch_check_stat) {
	case 0:
		ch1 = &(NP(ppi)->ch[PP_NP_EVT]);
		ch2 = &(NP(ppi)->ch[PP_NP_GEN]);
		break;
	case 1:
		ch1 = &(NP(ppi)->ch[PP_NP_GEN]);
		ch2 = &(NP(ppi)->ch[PP_NP_EVT]);
		break;
	default:
		/* Impossible! */
		break;
	}

	if (ch1->pkt_present)
		return recv(ch1->fd, pkt, len, 0);

	if (ch2->pkt_present)
		return recv(ch2->fd, pkt, len, 0);

	ch_check_stat = (ch_check_stat + 1) % 2;
	return -1;
}

int posix_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype,
	int use_pdelay_addr)
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PP_GENERAL_PORT);

	if (!use_pdelay_addr)
		addr.sin_addr.s_addr = NP(ppi)->mcast_addr;
	else
		addr.sin_addr.s_addr = NP(ppi)->peer_mcast_addr;

	return sendto(NP(ppi)->ch[chtype].fd, pkt, len, 0,
		(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
}

int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len, TimeInternal *t)
	__attribute__((alias("posix_recv_packet")));
int pp_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype,
	int use_pdelay_addr)
	__attribute__((alias("posix_send_packet")));

/* To open a channel we must bind to an interface and so on */
int posix_open_ch(struct pp_instance *ppi, char *ifname)
{
	/* TODO. In our first implementation, inherited by ptpd-2.1.0, we
	 * will handle UDP/IP packets. So, SOCK_DGRAM, IPPROTO_UDP, and all
	 * of standard UDP must be used when opening a socket. The port number
	 * should be passed to this function. (see posix-socket file too).
	 *
	 * Moreover, ptpd makes use of multicast, so setsockopt with
	 * IP_ADD_MEMBERSHIP must be called.
	 */
	int sock, iindex;
	struct ifreq ifr;
	struct sockaddr_ll addr;

	/* open socket */
	sock = socket(PF_PACKET, SOCK_RAW, PP_PROTO_NR);
	if (sock < 0) {
		pp_diag_error_str2(ppi, "socket()", strerror(errno));
		return -1;
	}

	/* hw interface information */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		pp_diag_error_str2(ppi, "SIOCGIFINDEX", strerror(errno));
		close(sock);
		return -1;
	}

	iindex = ifr.ifr_ifindex;
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
		pp_diag_error_str2(ppi, "SIOCGIFHWADDR", strerror(errno));
		close(sock);
		return -1;
	}
	/* FIXME: what to do with hw address */
	memcpy(NP(ppi)->ch[PP_NP_GEN].addr, ifr.ifr_hwaddr.sa_data, 6);
	memcpy(NP(ppi)->ch[PP_NP_EVT].addr, ifr.ifr_hwaddr.sa_data, 6);

	/* bind and setsockopt */
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(PP_PROTO_NR);
	addr.sll_ifindex = iindex;
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		pp_diag_error_str2(ppi, "bind", strerror(errno));
		close(sock);
		return -1;
	}
	/* FIXME: we are using the same socket for both channels by now */
	NP(ppi)->ch[PP_NP_GEN].fd = sock;
	NP(ppi)->ch[PP_NP_EVT].fd = sock;
	return 0;
}

/*
 * Inits all the network stuff
 */
int posix_net_init(struct pp_instance *ppi)
{
	char *ifname = "eth0"; /* TODO get it from rt_opts, or find interface if
				* not present in cmd line */

	/* TODO: how to handle udp ports? current pp_open_ch does not support
	 * it. Is it arch-specific? Actually, it is net protocol specific.
	 * We must decide how generic we want to be. Will we ever handle
	 * non-udp? Probably yes.
	 */
	posix_open_ch(ppi, ifname); /* FIXME: to be called twice, one for evt,
				    * one for general ? */
	return 0;
}

int pp_net_init(struct pp_instance *ppi)
	__attribute__((alias("posix_net_init")));


/*
 * Shutdown all the network stuff
 * TODO
 */
int posix_net_shutdown(struct pp_instance *ppi)
{
	return 0;
}

int posix_net_check_pkt(struct pp_instance *ppi, int delay_ms)
{
	fd_set set;
	int i;
	int ret = 0;
	int maxfd;
	struct posix_arch_data *arch_data =
		(struct posix_arch_data *)ppi->arch_data;

	if (delay_ms != -1) {
		/* Wait for a packet or for the timeout */
		arch_data->tv.tv_sec = delay_ms / 1000;
		arch_data->tv.tv_usec = (delay_ms % 1000) * 1000;
	}

	NP(ppi)->ch[PP_NP_GEN].pkt_present = 0;
	NP(ppi)->ch[PP_NP_EVT].pkt_present = 0;

	maxfd = (NP(ppi)->ch[PP_NP_GEN].fd > NP(ppi)->ch[PP_NP_EVT].fd) ?
		 NP(ppi)->ch[PP_NP_GEN].fd : NP(ppi)->ch[PP_NP_EVT].fd;
	FD_ZERO(&set);
	FD_SET(NP(ppi)->ch[PP_NP_GEN].fd, &set);
	FD_SET(NP(ppi)->ch[PP_NP_EVT].fd, &set);
	i = select(maxfd + 1, &set, NULL, NULL, &arch_data->tv);

	if (i < 0 && errno != EINTR)
		exit(__LINE__);

	if (i < 0) {
		ret = i;
		goto _end;
	}

	if (i == 0)
		goto _end;

	if (FD_ISSET(NP(ppi)->ch[PP_NP_GEN].fd, &set)) {
		ret++;
		NP(ppi)->ch[PP_NP_GEN].pkt_present = 1;
	}

	if (FD_ISSET(NP(ppi)->ch[PP_NP_EVT].fd, &set)) {
		ret++;
		NP(ppi)->ch[PP_NP_EVT].pkt_present = 1;
	}

_end:
	return ret;
}

int pp_net_shutdown(struct pp_instance *ppi)
__attribute__((alias("posix_net_shutdown")));
