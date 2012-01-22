/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
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

/* Uncomment one of the following */
#define PP_NET_UDP
/* #define PP_NET_IEEE_802_3 */


/* Receive and send is *not* so trivial */
int posix_recv_packet(struct pp_instance *ppi, void *pkt, int len,
	TimeInternal *t)
{
#ifdef PP_NET_UDP
	struct pp_channel *ch1 = NULL, *ch2 = NULL;

	if (POSIX_ARCH(ppi)->rcv_switch) {
		ch1 = &(NP(ppi)->ch[PP_NP_GEN]);
		ch2 = &(NP(ppi)->ch[PP_NP_EVT]);
	}
	else {
		ch1 = &(NP(ppi)->ch[PP_NP_EVT]);
		ch2 = &(NP(ppi)->ch[PP_NP_GEN]);
	}

	POSIX_ARCH(ppi)->rcv_switch = !POSIX_ARCH(ppi)->rcv_switch;

	if (ch1->pkt_present)
		return recv(ch1->fd, pkt, len, 0);

	if (ch2->pkt_present)
		return recv(ch2->fd, pkt, len, 0);

	return -1;
#endif /* PP_NET_UDP */

#ifdef PP_NET_IEEE_802_3
	/* TODO raw sockets */
#endif /* PP_NET_IEEE_802_3 */

	return -1;
}

int posix_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype,
	int use_pdelay_addr)
{
#ifdef PP_NET_UDP
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(chtype == PP_NP_GEN ? PP_GEN_PORT : PP_EVT_PORT);

	if (!use_pdelay_addr)
		addr.sin_addr.s_addr = NP(ppi)->mcast_addr;
	else
		addr.sin_addr.s_addr = NP(ppi)->peer_mcast_addr;

	return sendto(NP(ppi)->ch[chtype].fd, pkt, len, 0,
		(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
#endif /* PP_NET_UDP */

#ifdef PP_NET_IEEE_802_3
	/* TODO raw sockets */
#endif /* PP_NET_IEEE_802_3 */

	return -1;
}

int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len, TimeInternal *t)
	__attribute__((alias("posix_recv_packet")));
int pp_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype,
	int use_pdelay_addr)
	__attribute__((alias("posix_send_packet")));

/* To open a channel we must bind to an interface and so on */
int posix_open_ch(struct pp_instance *ppi, char *ifname, int chtype)
{

#ifdef PP_NET_UDP
	int sock = -1;
	int temp, iindex;
	struct in_addr iface_addr, net_addr;
	struct ifreq ifr;
	struct sockaddr_in addr;
	struct ip_mreq imr;
	char addr_str[INET_ADDRSTRLEN];

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		pp_diag_error_str2(ppi, "socket()", strerror(errno));
		return -1;
	}

	NP(ppi)->ch[chtype].fd = sock;

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

	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
		pp_diag_error_str2(ppi, "SIOCGIFADDR", strerror(errno));
		return -1;
	}

	memcpy(NP(ppi)->ch[chtype].addr, ifr.ifr_hwaddr.sa_data, 6);

	iface_addr.s_addr =
		((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

	PP_VPRINTF("Local IP address used : %s \n", inet_ntoa(iface_addr));

	temp = 1; /* allow address reuse */

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		       &temp, sizeof(int)) < 0) {
		pp_diag_error_str2(ppi, "SO_REUSEADDR", strerror(errno));
	}

	/* bind sockets */
	/* need INADDR_ANY to allow receipt of multi-cast and uni-cast
	 * messages */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(chtype == PP_NP_GEN ? PP_GEN_PORT : PP_EVT_PORT);
	if (bind(sock, (struct sockaddr *)&addr,
			sizeof(struct sockaddr_in)) < 0) {
		pp_diag_error_str2(ppi, "bind()", strerror(errno));
		return -1;
	}

	/* Init General multicast IP address */
	pp_memcpy(addr_str, PP_DEFAULT_DOMAIN_ADDRESS, INET_ADDRSTRLEN);

	if (!inet_aton(addr_str, &net_addr)) {
		pp_diag_error_str2(ppi, "inet_aton\n", strerror(errno));
		return -1;
	}
	NP(ppi)->mcast_addr = net_addr.s_addr;

	/* multicast send only on specified interface */
	imr.imr_multiaddr.s_addr = net_addr.s_addr;
	imr.imr_interface.s_addr = iface_addr.s_addr;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
		       &imr.imr_interface.s_addr, sizeof(struct in_addr)) < 0) {
		pp_diag_error_str2(ppi, "IP_MULTICAST_IF", strerror(errno));
		return -1;
	}

	/* join multicast group (for receiving) on specified interface */
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       &imr, sizeof(struct ip_mreq)) < 0) {
		pp_diag_error_str2(ppi, "IP_ADD_MEMBERSHIP", strerror(errno));
		return -1;
	}
	/* End of General multicast Ip address init */

	/* Init Peer multicast IP address */
	memcpy(addr_str, PP_PEER_DOMAIN_ADDRESS, INET_ADDRSTRLEN);

	if (!inet_aton(addr_str, &net_addr)) {
		pp_diag_error_str2(ppi, "inet_aton()", strerror(errno));
		return -1;
	}
	NP(ppi)->peer_mcast_addr = net_addr.s_addr;

	/* multicast send only on specified interface */
	imr.imr_multiaddr.s_addr = net_addr.s_addr;
	imr.imr_interface.s_addr = iface_addr.s_addr;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
		       &imr.imr_interface.s_addr, sizeof(struct in_addr)) < 0) {
		pp_diag_error_str2(ppi, "IP_MULTICAST_IF", strerror(errno));
		return -1;
	}
	/* join multicast group (for receiving) on specified interface */
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       &imr, sizeof(struct ip_mreq)) < 0) {
		pp_diag_error_str2(ppi, "IP_ADD_MEMBERSHIP", strerror(errno));
		return -1;
	}
	/* End of Peer multicast Ip address init */

	/* set socket time-to-live */
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
		       &OPTS(ppi)->ttl, sizeof(int)) < 0) {
		pp_diag_error_str2(ppi, "IP_MULTICAST_TTL", strerror(errno));
		return -1;
	}

	/* enable loopback */
	temp = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
		       &temp, sizeof(int)) < 0) {
		pp_diag_error_str2(ppi, "IP_MULTICAST_LOOP", strerror(errno));
		return -1;
	}

	/* make timestamps available through recvmsg() */
	if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP,
		       &temp, sizeof(int)) < 0) {
		pp_diag_error_str2(ppi, "SO_TIMESTAMP", strerror(errno));
		return -1;
	}

	NP(ppi)->ch[chtype].fd = sock;
	return 0;

#endif /* PP_NET_UDP */

#ifdef PP_NET_IEEE_802_3

	/* TODO: raw socket check open channel */
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

	memcpy(NP(ppi)->ch[chtype].addr, ifr.ifr_hwaddr.sa_data, 6);

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

	NP(ppi)->ch[chtype].fd = sock;
	return 0;
#endif /* PP_NET_IEEE_802_3 */

	return -1;
}

/*
 * Inits all the network stuff
 */
int posix_net_init(struct pp_instance *ppi)
{
	int i;
#ifdef PP_NET_UDP
	PP_PRINTF("posix_net_init UDP\n");
	for (i = PP_NP_GEN; i <= PP_NP_EVT; i++) {
		if (posix_open_ch(ppi, OPTS(ppi)->iface_name, i))
			return -1;
	}
	return 0;
#endif /* PP_NET_UDP */

#ifdef PP_NET_IEEE_802_3
	PP_PRINTF("posix_net_init IEEE 802.3\n");
	/* TODO raw sockets */
#endif /* PP_NET_IEEE_802_3 */

	PP_PRINTF("Error: pptp Compiled with unsupported network protocol!!\n");
	return -1;
}

int pp_net_init(struct pp_instance *ppi)
	__attribute__((alias("posix_net_init")));


/*
 * Shutdown all the network stuff
 */
int posix_net_shutdown(struct pp_instance *ppi)
{
#ifdef PP_NET_UDP
	struct ip_mreq imr;
	int fd;
	int i;

	for (i = PP_NP_GEN; i <= PP_NP_EVT; i++) {
		fd = NP(ppi)->ch[i].fd;
		if (fd < 0)
			continue;

		/* Close General Multicast */
		imr.imr_multiaddr.s_addr = NP(ppi)->mcast_addr;
		imr.imr_interface.s_addr = htonl(INADDR_ANY);

		setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
			&imr, sizeof(struct ip_mreq));

		/* Close Peer Multicast */
		imr.imr_multiaddr.s_addr = NP(ppi)->peer_mcast_addr;
		imr.imr_interface.s_addr = htonl(INADDR_ANY);

		setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
			   &imr, sizeof(struct ip_mreq));

		close(fd);

		NP(ppi)->ch[i].fd = -1;
	}

	NP(ppi)->mcast_addr = 0;
	NP(ppi)->peer_mcast_addr = 0;

	return 0;

#endif /* PP_NET_UDP */

#ifdef PP_NET_IEEE_802_3
	/* TODO raw sockets */
#endif /* PP_NET_IEEE_802_3 */

	return -1;
}

int posix_net_check_pkt(struct pp_instance *ppi, int delay_ms)
{
#ifdef PP_NET_UDP
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
#endif /* PP_NET_UDP */

#ifdef PP_NET_IEEE_802_3
	/* TODO raw sockets */
#endif /* PP_NET_IEEE_802_3 */

	return -1;
}

int pp_net_shutdown(struct pp_instance *ppi)
__attribute__((alias("posix_net_shutdown")));
