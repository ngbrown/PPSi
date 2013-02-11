/*
 * A simple tool to timestamp one tx and one rx packet
 * Alessandro Rubini 2011, GPL2 or later
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <net/ethernet.h>

#include "net_tstamp.h"
#include "misc-common.h"

#define BSIZE 2048

char pkt[BSIZE];

int main(int argc, char **argv)
{
	int sock;
	unsigned char macaddr[6];
	char *rest;
	int howto;

	if (argc != 3) {
		fprintf(stderr, "%s: Use \"%s <ifname> <bitmask>\n", argv[0],
			argv[0]);
		exit(1);
	}

	howto = strtol(argv[2], &rest, 0);
	if (rest && *rest) {
		fprintf(stderr, "%s: \"%s\" is not a number\n", argv[0],
			argv[2]);
		exit(1);
	}

	/* From ./net_tstamp.h, these are the "howto" values
	 *
	 * SOF_TIMESTAMPING_TX_HARDWARE = 1,
	 * SOF_TIMESTAMPING_TX_SOFTWARE = 2
	 * SOF_TIMESTAMPING_RX_HARDWARE = 4,
	 * SOF_TIMESTAMPING_RX_SOFTWARE = 8,
	 * SOF_TIMESTAMPING_SOFTWARE =   16,
	 * SOF_TIMESTAMPING_SYS_HARDWARE = 32,
	 * SOF_TIMESTAMPING_RAW_HARDWARE = 64,
	 */

	printf("%s: Using interface %s, with %i as SO_TIMESTAMPING\n",
	       argv[0], argv[1], howto);

	/* Create a socket to use for stamping, use the library code */
	sock = make_stamping_socket(stderr, argv[0], argv[1],
				     HWTSTAMP_TX_ON, HWTSTAMP_FILTER_ALL,
				     howto, macaddr, ETH_P_ALL);
	if (sock < 0) /* message already printed */
		exit(1);

	/* build a packet */
	memcpy(pkt + 0,  "\xff\xff\xff\xff\xff\xff", 6);
	memcpy(pkt + 6,  macaddr, 6);
	memcpy(pkt + 12, "xx", 2);

	if (send_and_stamp(sock, pkt, 64, 0) < 0) {
		fprintf(stderr, "%s: send_and_stamp: %s\n", argv[0],
			strerror(errno));
	} else {
		print_stamp(stdout, "tx", stderr);
	}
	if (recv_and_stamp(sock, pkt, sizeof(pkt), 0) < 0) {
		fprintf(stderr, "%s: recv_and_stamp: %s\n", argv[0],
			strerror(errno));
	} else {
		print_stamp(stdout, "rx", stderr);
	}
	exit(0);
}

