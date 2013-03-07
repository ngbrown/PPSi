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

#include <ppsi/ppsi.h>
#include "posix.h"

/* posix_recv_msg uses recvmsg for timestamp query */
static int posix_recv_msg(struct pp_instance *ppi, int fd, void *pkt, int len,
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
		t->seconds = tv->tv_sec;
		t->nanoseconds = tv->tv_usec * 1000;
		if (pp_verbose_time)
			pp_printf("%s: %9li.%06li\n", __func__, tv->tv_sec,
				  tv->tv_usec);
	} else {
		/*
		 * get the recording time here, even though it may  put a big
		 * spike in the offset signal sent to the clock servo
		 */
		PP_VPRINTF("no receive time stamp, getting it in user space\n");
		ppi->t_ops->get(ppi, t);
	}
	return ret;
}

/* Receive and send is *not* so trivial */
static int posix_net_recv(struct pp_instance *ppi, void *pkt, int len,
		   TimeInternal *t)
{
	struct pp_channel *ch1, *ch2;

	if (OPTS(ppi)->ethernet_mode) {
		int fd = NP(ppi)->ch[PP_NP_GEN].fd;

		return posix_recv_msg(ppi, fd, pkt, len, t);
	}

	/* else: UDP, we can return one frame only, so swap priority */
	if (POSIX_ARCH(ppi)->rcv_switch) {
		ch1 = &(NP(ppi)->ch[PP_NP_GEN]);
		ch2 = &(NP(ppi)->ch[PP_NP_EVT]);
	} else {
		ch1 = &(NP(ppi)->ch[PP_NP_EVT]);
		ch2 = &(NP(ppi)->ch[PP_NP_GEN]);
	}

	POSIX_ARCH(ppi)->rcv_switch = !POSIX_ARCH(ppi)->rcv_switch;

	if (ch1->pkt_present)
		return posix_recv_msg(ppi, ch1->fd, pkt, len, t);

	if (ch2->pkt_present)
		return posix_recv_msg(ppi, ch2->fd, pkt, len, t);

	return -1;
}

static int posix_net_send(struct pp_instance *ppi, void *pkt, int len,
			  TimeInternal *t, int chtype, int use_pdelay_addr)
{
	struct sockaddr_in addr;
	struct ethhdr *hdr = pkt;

	if (OPTS(ppi)->ethernet_mode) {
		hdr->h_proto = htons(ETH_P_1588);

		memcpy(hdr->h_dest, PP_MCAST_MACADDRESS, ETH_ALEN);
		/* raw socket implementation always uses gen socket */
		memcpy(hdr->h_source, NP(ppi)->ch[PP_NP_GEN].addr, ETH_ALEN);

		if (t)
			ppi->t_ops->get(ppi, t);

		return send(NP(ppi)->ch[PP_NP_GEN].fd, hdr, len, 0);
	}

	/* else: UDP */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(chtype == PP_NP_GEN ? PP_GEN_PORT : PP_EVT_PORT);

	addr.sin_addr.s_addr = NP(ppi)->mcast_addr;

	if (t)
		ppi->t_ops->get(ppi, t);

	return sendto(NP(ppi)->ch[chtype].fd, pkt, len, 0,
		(struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	return -1;
}

/* To open a channel we must bind to an interface and so on */
static int posix_open_ch(struct pp_instance *ppi, char *ifname, int chtype)
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

	if (OPTS(ppi)->ethernet_mode) {
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

	iindex = ifr.ifr_ifindex;
	context = "ioctl(SIOCGIFHWADDR)";
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
		goto err_out;

	memcpy(NP(ppi)->ch[chtype].addr, ifr.ifr_hwaddr.sa_data, 6);
	context = "ioctl(SIOCGIFADDR)";
	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
		goto err_out;

	iface_addr.s_addr =
		((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

	PP_VPRINTF("Local IP address used : %s\n", inet_ntoa(iface_addr));

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
	return -1;
}

static int posix_net_exit(struct pp_instance *ppi);

/*
 * Inits all the network stuff
 */

/* This function must be able to be called twice, and clean-up internally */
int posix_net_init(struct pp_instance *ppi)
{
	int i;

	if (NP(ppi)->ch[0].fd)
		posix_net_exit(ppi);

	/* The buffer is inside ppi, but we need to set pointers and align */
	pp_prepare_pointers(ppi);

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

/*
 * Shutdown all the network stuff
 */
static int posix_net_exit(struct pp_instance *ppi)
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

		close(fd);

		NP(ppi)->ch[i].fd = -1;
	}

	NP(ppi)->mcast_addr = 0;

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

struct pp_network_operations posix_net_ops = {
	.init = posix_net_init,
	.exit = posix_net_exit,
	.recv = posix_net_recv,
	.send = posix_net_send,
};
