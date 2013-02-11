#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>

#include "mtp.h"

#define MTP_PROTO 0x6d74  /* 'm' 't' */

struct eth_packet {
	struct ethhdr hdr;
	struct mtp_packet mtp;
};

static void run_passive_host(int argc, char **argv, int sock,
			     unsigned char *ourmac)
{
	int i;
	socklen_t slen;
	struct sockaddr_ll addr;
	struct timeval tv1, tv2;
	struct eth_packet pkt;

	/* get forward packet and stamp it */
	slen = sizeof(addr);
	i = recvfrom(sock, &pkt, sizeof(pkt), MSG_TRUNC,
		     (struct sockaddr *)&addr, &slen);
	gettimeofday(&tv1, NULL);
	if (i < 0) {
		fprintf(stderr, "%s: recvfrom(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	if (i < sizeof(pkt)) {
		fprintf(stderr, "%s: short packet 1\n", argv[0]);
		exit(1);
	}
	if (pkt.mtp.ptype != MTP_FORWARD) {
		fprintf(stderr, "%s: wrong packet 1\n", argv[0]);
		exit(1);
	}

	/* send backward packet -- swap macaddress */
	memcpy(pkt.hdr.h_dest, pkt.hdr.h_source, ETH_ALEN);
	memcpy(pkt.hdr.h_source, ourmac, ETH_ALEN);

	pkt.mtp.ptype = MTP_BACKWARD;
	gettimeofday(&tv2, NULL);
	send(sock, &pkt, sizeof(pkt), 0);

	/* send stamps */
	tv_to_ts(&tv1, pkt.mtp.t[1]);
	tv_to_ts(&tv2, pkt.mtp.t[2]);
	pkt.mtp.ptype = MTP_BACKSTAMP;
	send(sock, &pkt, sizeof(pkt), 0);
}

static int run_active_host(int argc, char **argv, int sock,
			   struct sockaddr_ll *addr, unsigned char *ourmac)
{
	int i, tragic;
	socklen_t slen;
	struct timeval tv0, tv3;
	struct eth_packet pkt;
	char othermac[ETH_ALEN];

	/* retrieve the remote mac*/
	if (sscanf(argv[2], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		   othermac+0, othermac+1, othermac+2,
		   othermac+3, othermac+4, othermac+5) != ETH_ALEN) {
		fprintf(stderr, "%s: %s: can't parse macaddress\n",
			argv[0], argv[2]);
		exit(1);
	}

	/* stamp and send the first packet */
	memset(&pkt, 0, sizeof(pkt));
	memcpy(pkt.hdr.h_dest, othermac, ETH_ALEN);
	memcpy(pkt.hdr.h_source, ourmac, ETH_ALEN);
	pkt.hdr.h_proto = addr->sll_protocol;
	pkt.mtp.ptype = MTP_FORWARD;
	srand(time(NULL));
	pkt.mtp.tragic = tragic = rand();
	gettimeofday(&tv0, NULL);
	send(sock, &pkt, sizeof(pkt), 0);

	/* get the second packet -- and discard it */
	slen = sizeof(*addr);
	i = recvfrom(sock, &pkt, sizeof(pkt), MSG_TRUNC,
		     (struct sockaddr *)addr, &slen);
	gettimeofday(&tv3, NULL);
	if (i < 0) {
		fprintf(stderr, "%s: recvfrom(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	if (i < sizeof(pkt)) {
		fprintf(stderr, "%s: short packet 1\n", argv[0]);
		exit(1);
	}
	if (pkt.mtp.ptype != MTP_BACKWARD || pkt.mtp.tragic != tragic) {
		fprintf(stderr, "%s: wrong packet 1\n", argv[0]);
		exit(1);
	}

	/* get the final packet */
	slen = sizeof(*addr);
	i = recvfrom(sock, &pkt, sizeof(pkt), MSG_TRUNC,
		     (struct sockaddr *)addr, &slen);
	if (i < 0) {
		fprintf(stderr, "%s: recvfrom(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	if (i < sizeof(pkt)) {
		fprintf(stderr, "%s: short packet 2\n", argv[0]);
		exit(1);
	}
	if (pkt.mtp.ptype != MTP_BACKSTAMP || pkt.mtp.tragic != tragic) {
		fprintf(stderr, "%s: wrong packet 2\n", argv[0]);
		exit(1);
	}

	/* add our stamp, we are using only the first value */
	tv_to_ts(&tv0, pkt.mtp.t[0]);
	tv_to_ts(&tv3, pkt.mtp.t[3]);

	mtp_result(&pkt.mtp);
	return 0;
}

int main(int argc, char **argv)
{
	int sock, iindex, mtp_listen = 0;
	struct sockaddr_ll addr;
	struct ifreq ifr;
	unsigned char ourmac[ETH_ALEN];

	if (argc != 3) {
		fprintf(stderr, "%s: Use: \"%s <eth> -l\" or "
			"\"%s <eth> <macaddress>\"\n",
			argv[0], argv[0], argv[0]);
		exit(1);
	}
	if (!strcmp(argv[2], "-l")) {
		mtp_listen = 1;
	}

	/* open file descriptor */
	sock = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL); //htons(MTP_PROTO));
	if (sock < 0) {
		fprintf(stderr, "%s: socket(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	/* get interface index */
	memset (&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, argv[1]);
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		fprintf(stderr, "%s: SIOCGIFINDEX(%s): %s\n", argv[0],
			argv[1], strerror(errno));
		exit(1);
	}
	iindex = ifr.ifr_ifindex;

	/* get our mac address */
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
		fprintf(stderr, "%s: SIOCGIFHWADDR(%s): %s\n", argv[0],
			argv[1], strerror(errno));
		exit(1);
	}
	memcpy(ourmac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	/* bind to a port, so we can get data  */
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(MTP_PROTO);
	addr.sll_ifindex = iindex;
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "%s: bind(%s): %s\n", argv[0], argv[1],
			strerror(errno));
		exit(1);
	}

	if (!mtp_listen)
		return run_active_host(argc, argv, sock, &addr, ourmac);

	while (1)
		run_passive_host(argc, argv, sock, ourmac);
}

