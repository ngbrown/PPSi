/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/* Socket interface for GNU/Linux (and most likely other posix systems */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <pproto/pproto.h>
#include <pproto/diag.h>
#include "posix.h"


/* Receive and send is trivial */
int posix_recv_packet(struct pp_instance *ppi, void *pkt, int len)
{
	return recv(ppi->ch.fd, pkt, len, 0);
}

int posix_send_packet(struct pp_instance *ppi, void *pkt, int len)
{
	return send(ppi->ch.fd, pkt, len, 0);
}

int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len)
	__attribute__((alias("posix_recv_packet")));
int pp_send_packet(struct pp_instance *ppi, void *pkt, int len)
	__attribute__((alias("posix_send_packet")));

/* To open a channel we must bind to an interface and so on */
int posix_open_ch(struct pp_instance *ppi, char *ifname)
{
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
	memset (&ifr, 0, sizeof(ifr));
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
	memcpy(ppi->ch.addr, ifr.ifr_hwaddr.sa_data, 6);

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

	ppi->ch.fd = sock;
	return 0;
}

/*
 * Inits all the network stuff
 * TODO: check with the posix_open_ch, maybe it's the same
 */
int posix_net_init(struct pp_instance *ppi)
{
	return 0;
}

int pp_net_init(struct pp_instance *ppi)
	__attribute__((alias("posix_net_init")));
