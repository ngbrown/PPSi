/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

/* Socket interface for GNU/Linux (and most likely other posix systems) */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include <pptp/pptp.h>
#include <pptp/diag.h>
#include "posix.h"

/* posix_recv_msg uses recvmsg for timestamp query */
int posix_recv_msg(int fd, void *pkt, int len, TimeInternal *t)
{
	ssize_t ret;
	struct msghdr msg;
	struct iovec vec[1];

	union {
		struct cmsghdr cm;
		char control[512];
	} cmsg_un;

	struct cmsghdr *cmsg;
	struct timeval *tv;

	vec[0].iov_base = pkt;
	vec[0].iov_len = PP_PACKET_SIZE;

	memset(&msg, 0, sizeof(msg));
	memset(&cmsg_un, 0, sizeof(cmsg_un));

	/* msg_name, msg_namelen == 0: not used */
	msg.msg_iov = vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsg_un.control;
	msg.msg_controllen = sizeof(cmsg_un.control);

	ret = recvmsg(fd, &msg, MSG_DONTWAIT);
	if (ret <= 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		return ret;
	}
	if (msg.msg_flags & MSG_TRUNC) {
		PP_PRINTF("Error: received truncated message\n");
		return 0;
	}
	/* get time stamp of packet */
	if (msg.msg_flags & MSG_CTRUNC) {
		PP_PRINTF("Error: received truncated ancillary data\n");
		return 0;
	}

	tv = NULL;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
	     cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_SOCKET &&
		    cmsg->cmsg_type == SCM_TIMESTAMP)
			tv = (struct timeval *)CMSG_DATA(cmsg);
	}

	if (tv) {
		t->seconds = tv->tv_sec;
		t->nanoseconds = tv->tv_usec * 1000;
		PP_VPRINTF("kernel recv time stamp %us %dns\n",
		     t->seconds, t->nanoseconds);
	} else {
		/*
		 * do not try to get by with recording the time here, better
		 * to fail because the time recorded could be well after the
		 * message receive, which would put a big spike in the
		 * offset signal sent to the clock servo
		 */
		PP_VPRINTF("no receive time stamp\n");
		return 0;
	}
	return ret;
}

/* Receive and send is *not* so trivial */
int posix_recv_packet(struct pp_instance *ppi, void *pkt, int len,
	TimeInternal *t)
{
	struct pp_channel *ch1 = NULL, *ch2 = NULL;
	void *hdr;
	int ret;

	if (OPTS(ppi)->ethernet_mode) {
		hdr = PROTO_HDR(pkt);
		ret = posix_recv_msg(NP(ppi)->ch[PP_NP_GEN].fd, hdr,
				      len + NP(ppi)->proto_ofst, t);
		return ret <= 0 ? ret : ret - NP(ppi)->proto_ofst;
		/* FIXME: check header */
	}

	/* else: UDP */
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
		return posix_recv_msg(ch1->fd, pkt, len, t);

	if (ch2->pkt_present)
		return posix_recv_msg(ch2->fd, pkt, len, t);

	return -1;
}

int posix_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype,
	int use_pdelay_addr)
{
	struct sockaddr_in addr;
	struct ethhdr *hdr;

	if (OPTS(ppi)->ethernet_mode) {
		hdr = PROTO_HDR(pkt);

		hdr->h_proto = htons(ETH_P_1588);

		if (OPTS(ppi)->gptp_mode)
			memcpy(hdr->h_dest, PP_PEER_MACADDRESS, ETH_ALEN);
		else
			memcpy(hdr->h_dest, PP_MCAST_MACADDRESS, ETH_ALEN);
		/* raw socket implementation always uses gen socket */
		memcpy(hdr->h_source, NP(ppi)->ch[PP_NP_GEN].addr, ETH_ALEN);
		return send(NP(ppi)->ch[PP_NP_GEN].fd, hdr,
					len + NP(ppi)->proto_ofst, 0);
	}

	/* else: UDP */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(chtype == PP_NP_GEN ? PP_GEN_PORT : PP_EVT_PORT);

	if (!use_pdelay_addr)
		addr.sin_addr.s_addr = NP(ppi)->mcast_addr;
	else
		addr.sin_addr.s_addr = NP(ppi)->peer_mcast_addr;

	return sendto(NP(ppi)->ch[chtype].fd, pkt, len, 0,
		(struct sockaddr *)&addr, sizeof(struct sockaddr_in));

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

	int sock = -1;
	int temp, iindex;
	struct in_addr iface_addr, net_addr;
	struct ifreq ifr;
	struct sockaddr_in addr;
	struct sockaddr_ll addr_ll;
	struct ip_mreq imr;
	struct packet_mreq pmr;
	char addr_str[INET_ADDRSTRLEN];

	if (OPTS(ppi)->ethernet_mode) {
		/* open socket */
		sock = socket(PF_PACKET, SOCK_RAW, ETH_P_1588);
		if (sock < 0) {
			pp_diag_error_str2(ppi, "socket()", strerror(errno));
			return -1;
		}

		/* hw interface information */
		memset(&ifr, 0, sizeof(ifr));
		strcpy(ifr.ifr_name, ifname);
		if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
			pp_diag_error_str2(ppi, "SIOCGIFINDEX",
					   strerror(errno));
			close(sock);
			return -1;
		}

		iindex = ifr.ifr_ifindex;
		if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
			pp_diag_error_str2(ppi, "SIOCGIFHWADDR",
					   strerror(errno));
			close(sock);
			return -1;
		}

		memcpy(NP(ppi)->ch[chtype].addr, ifr.ifr_hwaddr.sa_data, 6);

		/* bind */
		memset(&addr_ll, 0, sizeof(addr));
		addr_ll.sll_family = AF_PACKET;
		addr_ll.sll_protocol = htons(ETH_P_1588);
		addr_ll.sll_ifindex = iindex;
		if (bind(sock, (struct sockaddr *)&addr_ll,
			 sizeof(addr_ll)) < 0) {
			pp_diag_error_str2(ppi, "bind", strerror(errno));
			close(sock);
			return -1;
		}

		/* accept the multicast address for raw-ethernet ptp */
		memset(&pmr, 0, sizeof(pmr));
		pmr.mr_ifindex = iindex;
		pmr.mr_type = PACKET_MR_MULTICAST;
		pmr.mr_alen = ETH_ALEN;
		memcpy(pmr.mr_address, PP_MCAST_MACADDRESS, ETH_ALEN);
		setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
			   &pmr, sizeof(pmr)); /* lazily ignore errors */

		/* also the PEER multicast address */
		memcpy(pmr.mr_address, PP_PEER_MACADDRESS, ETH_ALEN);
		setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
			   &pmr, sizeof(pmr)); /* lazily ignore errors */

		NP(ppi)->ch[chtype].fd = sock;

		/* make timestamps available through recvmsg() -- FIXME: hw? */
		setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP,
			   &temp, sizeof(int));

		return 0;
	}

	/* else: UDP */
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

	memcpy(NP(ppi)->ch[chtype].addr, ifr.ifr_hwaddr.sa_data, 6);

	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
		pp_diag_error_str2(ppi, "SIOCGIFADDR", strerror(errno));
		return -1;
	}

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

	/* multicast sends only on specified interface */
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

	/* multicast sends only on specified interface */
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
}

/*
 * Inits all the network stuff
 */
int posix_net_init(struct pp_instance *ppi)
{
	int i;

	ppi->buf_out = calloc(1, PP_PACKET_SIZE + NP(ppi)->proto_ofst);
	if (!ppi->buf_out)
		return -1;

	ppi->buf_out = PROTO_PAYLOAD(ppi->buf_out);

	if (OPTS(ppi)->ethernet_mode) {
		PP_PRINTF("posix_net_init IEEE 802.3\n");

		/* raw sockets implementation always use gen socket */
		return posix_open_ch(ppi, OPTS(ppi)->iface_name, PP_NP_GEN);
	}

	/* else: UDP */
	PP_PRINTF("posix_net_init UDP\n");
	for (i = PP_NP_GEN; i <= PP_NP_EVT; i++) {
		if (posix_open_ch(ppi, OPTS(ppi)->iface_name, i))
			return -1;
	}
	return 0;
}

int pp_net_init(struct pp_instance *ppi)
	__attribute__((alias("posix_net_init")));


/*
 * Shutdown all the network stuff
 */
int posix_net_shutdown(struct pp_instance *ppi)
{
	struct ip_mreq imr;
	int fd;
	int i;

	if (OPTS(ppi)->ethernet_mode) {
		close(NP(ppi)->ch[PP_NP_GEN].fd);
		return 0;
	}

	/* else: UDP */
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

	if (OPTS(ppi)->ethernet_mode) {
		maxfd = NP(ppi)->ch[PP_NP_GEN].fd;
		FD_ZERO(&set);
		FD_SET(NP(ppi)->ch[PP_NP_GEN].fd, &set);

		i = select(maxfd + 1, &set, NULL, NULL, &arch_data->tv);

		if (i < 0 && errno != EINTR)
			exit(__LINE__);

		if (i < 0)
			return -1;

		if (i == 0)
			return 0;

		NP(ppi)->ch[PP_NP_GEN].pkt_present = 1;
		return 1;
	}

	/* else: UDP */
	maxfd = (NP(ppi)->ch[PP_NP_GEN].fd > NP(ppi)->ch[PP_NP_EVT].fd) ?
		 NP(ppi)->ch[PP_NP_GEN].fd : NP(ppi)->ch[PP_NP_EVT].fd;
	FD_ZERO(&set);
	FD_SET(NP(ppi)->ch[PP_NP_GEN].fd, &set);
	FD_SET(NP(ppi)->ch[PP_NP_EVT].fd, &set);
	i = select(maxfd + 1, &set, NULL, NULL, &arch_data->tv);

	if (i < 0 && errno != EINTR)
		exit(__LINE__);

	if (i < 0)
		return -1;

	if (i == 0)
		return 0;

	if (FD_ISSET(NP(ppi)->ch[PP_NP_GEN].fd, &set)) {
		ret++;
		NP(ppi)->ch[PP_NP_GEN].pkt_present = 1;
	}

	if (FD_ISSET(NP(ppi)->ch[PP_NP_EVT].fd, &set)) {
		ret++;
		NP(ppi)->ch[PP_NP_EVT].pkt_present = 1;
	}

	return ret;
}

int pp_net_shutdown(struct pp_instance *ppi)
__attribute__((alias("posix_net_shutdown")));
