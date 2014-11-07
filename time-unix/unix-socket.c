/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
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

#include <ppsi/ppsi.h>
#include "ptpdump.h"
#include "../arch-unix/ppsi-unix.h"

/* unix_recv_msg uses recvmsg for timestamp query */
static int unix_recv_msg(struct pp_instance *ppi, int fd, void *pkt, int len,
			  TimeInternal *t)
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
	vec[0].iov_len = PP_MAX_FRAME_LENGTH;

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
		pp_error("%s: truncated message\n", __func__);
		return 0;
	}
	/* get time stamp of packet */
	if (msg.msg_flags & MSG_CTRUNC) {
		pp_error("%s: truncated ancillary data\n", __func__);
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
		t->seconds = tv->tv_sec + DSPRO(ppi)->currentUtcOffset;
		t->nanoseconds = tv->tv_usec * 1000;
		t->correct = 1;
	} else {
		/*
		 * get the recording time here, even though it may  put a big
		 * spike in the offset signal sent to the clock servo
		 */
		ppi->t_ops->get(ppi, t);
	}

	if (ppsi_drop_rx()) {
		pp_diag(ppi, frames, 1, "Drop received frame\n");
		return -2;
	}

	/* This is not really hw... */
	pp_diag(ppi, time, 2, "recv stamp: %i.%09i (%s)\n",
		(int)t->seconds, (int)t->nanoseconds, tv ? "kernel" : "user");
	return ret;
}

/* Receive and send is *not* so trivial */
static int unix_net_recv(struct pp_instance *ppi, void *pkt, int len,
		   TimeInternal *t)
{
	struct pp_channel *ch1, *ch2;
	int ret;

	if (ppi->proto == PPSI_PROTO_RAW) {
		int fd = NP(ppi)->ch[PP_NP_GEN].fd;

		ret = unix_recv_msg(ppi, fd, pkt, len, t);
		if (ret > 0 && pp_diag_allow(ppi, frames, 2))
			dump_1588pkt("recv: ", pkt, ret, t);
		return ret;
	}

	/* else: UDP, we can return one frame only, always handle EVT msgs
	 * before GEN */
	ch1 = &(NP(ppi)->ch[PP_NP_EVT]);
	ch2 = &(NP(ppi)->ch[PP_NP_GEN]);

	ret = -1;
	if (ch1->pkt_present)
		ret = unix_recv_msg(ppi, ch1->fd, pkt, len, t);
	else if (ch2->pkt_present)
		ret = unix_recv_msg(ppi, ch2->fd, pkt, len, t);

	if (ret > 0 && pp_diag_allow(ppi, frames, 2))
		dump_payloadpkt("recv: ", pkt, ret, t);
	return ret;
}

static int unix_net_send(struct pp_instance *ppi, void *pkt, int len,
			  TimeInternal *t, int chtype, int use_pdelay_addr)
{
	struct sockaddr_in addr;
	struct ethhdr *hdr = pkt;
	int ret;

	/* To fake a network frame loss, set the timestamp and do not send */
	if (ppsi_drop_tx()) {
		if (t)
			ppi->t_ops->get(ppi, t);
		pp_diag(ppi, frames, 1, "Drop sent frame\n");
		return len;
	}

	if (ppi->proto == PPSI_PROTO_RAW) {
		hdr->h_proto = htons(ETH_P_1588);

		memcpy(hdr->h_dest, PP_MCAST_MACADDRESS, ETH_ALEN);
		/* raw socket implementation always uses gen socket */
		memcpy(hdr->h_source, NP(ppi)->ch[PP_NP_GEN].addr, ETH_ALEN);

		if (t)
			ppi->t_ops->get(ppi, t);

		ret = send(NP(ppi)->ch[PP_NP_GEN].fd, hdr, len, 0);
		if (pp_diag_allow(ppi, frames, 2))
			dump_1588pkt("send: ", pkt, len, t);
		return ret;
	}

	/* else: UDP */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(chtype == PP_NP_GEN ? PP_GEN_PORT : PP_EVT_PORT);

	addr.sin_addr.s_addr = NP(ppi)->mcast_addr;

	if (t)
		ppi->t_ops->get(ppi, t);

	ret = sendto(NP(ppi)->ch[chtype].fd, pkt, len, 0,
		(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (pp_diag_allow(ppi, frames, 2))
		dump_payloadpkt("send: ", pkt, len, t);
	return ret;
}

/* To open a channel we must bind to an interface and so on */
static int unix_open_ch(struct pp_instance *ppi, char *ifname, int chtype)
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
	char *context;

	if (ppi->proto == PPSI_PROTO_RAW) {
		/* open socket */
		context = "socket()";
		sock = socket(PF_PACKET, SOCK_RAW, ETH_P_1588);
		if (sock < 0)
			goto err_out;

		/* hw interface information */
		memset(&ifr, 0, sizeof(ifr));
		strcpy(ifr.ifr_name, ifname);
		context = "ioctl(SIOCGIFINDEX)";
		if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
			goto err_out;

		iindex = ifr.ifr_ifindex;
		context = "ioctl(SIOCGIFHWADDR)";
		if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
			goto err_out;

		memcpy(NP(ppi)->ch[chtype].addr, ifr.ifr_hwaddr.sa_data, 6);

		/* bind */
		memset(&addr_ll, 0, sizeof(addr));
		addr_ll.sll_family = AF_PACKET;
		addr_ll.sll_protocol = htons(ETH_P_1588);
		addr_ll.sll_ifindex = iindex;
		context = "bind()";
		if (bind(sock, (struct sockaddr *)&addr_ll,
			 sizeof(addr_ll)) < 0)
			goto err_out;

		/* accept the multicast address for raw-ethernet ptp */
		memset(&pmr, 0, sizeof(pmr));
		pmr.mr_ifindex = iindex;
		pmr.mr_type = PACKET_MR_MULTICAST;
		pmr.mr_alen = ETH_ALEN;
		memcpy(pmr.mr_address, PP_MCAST_MACADDRESS, ETH_ALEN);
		setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
			   &pmr, sizeof(pmr)); /* lazily ignore errors */

		NP(ppi)->ch[chtype].fd = sock;

		/* make timestamps available through recvmsg() -- FIXME: hw? */
		setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP,
			   &temp, sizeof(int));

		return 0;
	}

	/* else: UDP */
	context = "socket()";
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
		goto err_out;

	NP(ppi)->ch[chtype].fd = sock;

	/* hw interface information */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);
	context = "ioctl(SIOCGIFINDEX)";
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
		goto err_out;

	context = "ioctl(SIOCGIFHWADDR)";
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
		goto err_out;

	memcpy(NP(ppi)->ch[chtype].addr, ifr.ifr_hwaddr.sa_data, 6);
	context = "ioctl(SIOCGIFADDR)";
	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
		goto err_out;

	iface_addr.s_addr =
		((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

	pp_diag(ppi, frames, 2, "Local IP address used : %s\n",
			inet_ntoa(iface_addr));

	temp = 1; /* allow address reuse */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		       &temp, sizeof(int)) < 0)
		pp_printf("%s: ioctl(SO_REUSEADDR): %s\n", __func__,
			  strerror(errno));

	/* bind sockets */
	/* need INADDR_ANY to allow receipt of multi-cast and uni-cast
	 * messages */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(chtype == PP_NP_GEN ? PP_GEN_PORT : PP_EVT_PORT);
	context = "bind()";
	if (bind(sock, (struct sockaddr *)&addr,
		 sizeof(struct sockaddr_in)) < 0)
		goto err_out;

	/* Init General multicast IP address */
	memcpy(addr_str, PP_DEFAULT_DOMAIN_ADDRESS, INET_ADDRSTRLEN);

	context = addr_str; errno = EINVAL;
	if (!inet_aton(addr_str, &net_addr))
		goto err_out;
	NP(ppi)->mcast_addr = net_addr.s_addr;

	/* multicast sends only on specified interface */
	imr.imr_multiaddr.s_addr = net_addr.s_addr;
	imr.imr_interface.s_addr = iface_addr.s_addr;
	context = "setsockopt(IP_MULTICAST_IF)";
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
		       &imr.imr_interface.s_addr, sizeof(struct in_addr)) < 0)
		goto err_out;

	/* join multicast group (for receiving) on specified interface */
	context = "setsockopt(IP_ADD_MEMBERSHIP)";
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       &imr, sizeof(struct ip_mreq)) < 0)
		goto err_out;
	/* End of General multicast Ip address init */

	/* set socket time-to-live */
	context = "setsockopt(IP_MULTICAST_TTL)";
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
		       &OPTS(ppi)->ttl, sizeof(int)) < 0)
		goto err_out;

	/* forcibly disable loopback */
	temp = 0;
	context = "setsockopt(IP_MULTICAST_LOOP)";
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
		       &temp, sizeof(int)) < 0)
		goto err_out;

	/* make timestamps available through recvmsg() */
	context = "setsockopt(SO_TIMESTAMP)";
	if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP,
		       &temp, sizeof(int)) < 0)
		goto err_out;

	NP(ppi)->ch[chtype].fd = sock;
	return 0;

err_out:
	pp_printf("%s: %s: %s\n", __func__, context, strerror(errno));
	if (sock >= 0)
		close(sock);
	NP(ppi)->ch[chtype].fd = -1;
	return -1;
}

static int unix_net_exit(struct pp_instance *ppi);

/*
 * Inits all the network stuff
 */

/* This function must be able to be called twice, and clean-up internally */
static int unix_net_init(struct pp_instance *ppi)
{
	int i;

	if (NP(ppi)->ch[0].fd > 0)
		unix_net_exit(ppi);

	/* The buffer is inside ppi, but we need to set pointers and align */
	pp_prepare_pointers(ppi);

	if (ppi->proto == PPSI_PROTO_RAW) {
		pp_diag(ppi, frames, 1, "unix_net_init IEEE 802.3\n");

		/* raw sockets implementation always use gen socket */
		return unix_open_ch(ppi, ppi->iface_name, PP_NP_GEN);
	}

	/* else: UDP */
	pp_diag(ppi, frames, 1, "unix_net_init UDP\n");
	for (i = PP_NP_GEN; i <= PP_NP_EVT; i++) {
		if (unix_open_ch(ppi, ppi->iface_name, i))
			return -1;
	}

	return 0;
}

/*
 * Shutdown all the network stuff
 */
static int unix_net_exit(struct pp_instance *ppi)
{
	struct ip_mreq imr;
	int fd;
	int i;

	if (ppi->proto == PPSI_PROTO_RAW) {
		fd = NP(ppi)->ch[PP_NP_GEN].fd;
		if (fd > 0) {
			close(fd);
			NP(ppi)->ch[PP_NP_GEN].fd = -1;
		}
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

		close(fd);

		NP(ppi)->ch[i].fd = -1;
	}

	NP(ppi)->mcast_addr = 0;

	return 0;
}

static int unix_net_check_packet(struct pp_globals *ppg, int delay_ms)
{
	fd_set set;
	int i, j, k;
	int ret = 0;
	int maxfd = -1;
	struct unix_arch_data *arch_data = POSIX_ARCH(ppg);
	int old_delay_ms;

	old_delay_ms = arch_data->tv.tv_sec * 1000 +
		arch_data->tv.tv_usec / 1000;

	if ((delay_ms != -1) &&
		((old_delay_ms == 0) || (delay_ms < old_delay_ms))) {
		/* Wait for a packet or for the timeout */
		arch_data->tv.tv_sec = delay_ms / 1000;
		arch_data->tv.tv_usec = (delay_ms % 1000) * 1000;
	}

	/* Detect general timeout with no needs for select stuff */
	if ((arch_data->tv.tv_sec == 0) && (arch_data->tv.tv_usec == 0))
		return 0;

	FD_ZERO(&set);

	for (j = 0; j < ppg->nlinks; j++) {
		struct pp_instance *ppi = INST(ppg, j);
		int fd_to_set;

		/* Use either fd that is valid, irrespective of ether/udp */
		for (k = 0; k < 2; k++) {
			NP(ppi)->ch[k].pkt_present = 0;
			fd_to_set = NP(ppi)->ch[k].fd;
			if (fd_to_set < 0)
				continue;

			FD_SET(fd_to_set, &set);
			maxfd = fd_to_set > maxfd ? fd_to_set : maxfd;
		}
	}
	i = select(maxfd + 1, &set, NULL, NULL, &arch_data->tv);

	if (i < 0 && errno != EINTR)
		exit(__LINE__);

	if (i < 0)
		return -1;

	if (i == 0)
		return 0;

	for (j = 0; j < ppg->nlinks; j++) {
		struct pp_instance *ppi = INST(ppg, j);
		int fd = NP(ppi)->ch[PP_NP_GEN].fd;

		if (fd >= 0 && FD_ISSET(fd, &set)) {
			ret++;
			NP(ppi)->ch[PP_NP_GEN].pkt_present = 1;
		}

		fd = NP(ppi)->ch[PP_NP_EVT].fd;

		if (fd >= 0 && FD_ISSET(fd, &set)) {
			ret++;
			NP(ppi)->ch[PP_NP_EVT].pkt_present = 1;
		}
	}
	return ret;
}

struct pp_network_operations unix_net_ops = {
	.init = unix_net_init,
	.exit = unix_net_exit,
	.recv = unix_net_recv,
	.send = unix_net_send,
	.check_packet = unix_net_check_packet,
};


