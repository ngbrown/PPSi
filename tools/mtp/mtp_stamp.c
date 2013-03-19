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
#include "net_tstamp.h" /* copied from Linux headers */
#include "misc-common.h"

#define MTP_PROTO 0x7878 //0x6d74  /* 'm' 't' */

struct eth_packet {
	struct ethhdr hdr;
	struct mtp_packet mtp;
};

static void run_passive_host(int argc, char **argv, int sock,
			     unsigned char *ourmac)
{
	int i;
	struct timespec ts1[4], ts2[4];
	struct eth_packet pkt;

	/* get forward packet and stamp it */
	i = recv_and_stamp(sock, &pkt, sizeof(pkt), MSG_TRUNC);
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
	get_stamp(ts1); /* the 4 stamps collected by recv_and_stamp() */

	/* Wait a while, so our peer can get the tx-done interrupt */
	usleep(100);

	/* send backward packet -- swap macaddress */
	memcpy(pkt.hdr.h_dest, pkt.hdr.h_source, ETH_ALEN);
	memcpy(pkt.hdr.h_source, ourmac, ETH_ALEN);

	pkt.mtp.ptype = MTP_BACKWARD;
	send_and_stamp(sock, &pkt, sizeof(pkt), 0);
	get_stamp(ts2); /* the 4 stamps collected by recv_and_stamp() */

	/*
	 * fill the packet: the first ts is the same as the second
	 * (see onestamp output), so only copy the next three
	 */
	memcpy(pkt.mtp.t[1], ts1+1, sizeof(pkt.mtp.t[1]));
	memcpy(pkt.mtp.t[2], ts2+1, sizeof(pkt.mtp.t[2]));
	pkt.mtp.ptype = MTP_BACKSTAMP;
	/* se don't use the stamp but the message queue must be emptied */
	send_and_stamp(sock, &pkt, sizeof(pkt), 0);
}

static int run_active_host(int argc, char **argv, int sock,
			   unsigned char *ourmac)
{
	int i, tragic;
	struct timespec ts0[4], ts3[4];
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
	pkt.hdr.h_proto = ntohs(MTP_PROTO);
	pkt.mtp.ptype = MTP_FORWARD;
	srand(time(NULL));
	pkt.mtp.tragic = tragic = rand();
	send_and_stamp(sock, &pkt, sizeof(pkt), 0);
	get_stamp(ts0);

	/* get the second packet -- and discard it */
	i = recv_and_stamp(sock, &pkt, sizeof(pkt), MSG_TRUNC);
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
	get_stamp(ts3);

	/* get the final packet */
	i = recv(sock, &pkt, sizeof(pkt), MSG_TRUNC);
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
	memcpy(pkt.mtp.t[0], ts0+1, sizeof(pkt.mtp.t[0]));
	memcpy(pkt.mtp.t[3], ts3+1, sizeof(pkt.mtp.t[3]));

	mtp_result(&pkt.mtp);
	return 0;
}

int main(int argc, char **argv)
{
	int sock, mtp_listen = 0;
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
	sock = make_stamping_socket(stderr, argv[0], argv[1],
				    HWTSTAMP_TX_ON, HWTSTAMP_FILTER_ALL,
				    127 /* all bits for stamping */,
				    ourmac, MTP_PROTO);
	if (sock < 0) /* message already printed */
		exit(1);

	if (!mtp_listen)
		return run_active_host(argc, argv, sock, ourmac);

	while (1)
		run_passive_host(argc, argv, sock, ourmac);
}

