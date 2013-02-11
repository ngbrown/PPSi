/*
 * A simple tool to test packet transmission, with or without hw stamping
 * Alessandro Rubini 2011, GPL2 or later
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "net_tstamp.h"
#include "misc-common.h"

#define BSIZE 2048 /* Packet buffer */

static int howto = 1;

static char pkt[2][BSIZE];


int main(int argc, char **argv)
{
	int sock[2];
	unsigned char macaddr[2][6];
	int i, j, k, nloop=1000;

	if (argc != 3) {
		fprintf(stderr, "%s: Use: \"%s <ethname> <ethname>\"\n",
			argv[0], argv[0]);
		exit(1);
	}
	/* Extra arguments from environment */
	if (getenv("STAMP_NLOOP"))
		nloop = atoi(getenv("STAMP_NLOOP"));
	if (getenv("STAMP_HOWTO"))
		howto = atoi(getenv("STAMP_HOWTO"));
	fprintf(stderr, "%s: working on %s and %s, %i loops, %02x stamp\n",
	       argv[0], argv[1], argv[2], nloop, howto);

	/* Create two sockets */
	sock[0] = make_stamping_socket(stderr, argv[0], argv[1],
				     HWTSTAMP_TX_ON, HWTSTAMP_FILTER_ALL,
				     howto, macaddr[0]);
	if (sock[0] < 0)
		exit(1);
	sock[1] = make_stamping_socket(stderr, argv[0], argv[2],
				     HWTSTAMP_TX_ON, HWTSTAMP_FILTER_ALL,
				     howto, macaddr[1]);
	if (sock[1] < 0)
		exit(1);

	/* Build the packet */
	memcpy(pkt[0]+0,  macaddr[1], 6);
	memcpy(pkt[0]+6,  macaddr[0], 6);
	pkt[0][12] = ETH_P_1588 >> 8;
	pkt[0][13] = ETH_P_1588 & 0xff;

	for (i = 64; i < 1500; i += 64) {
		/* there are 4 timespecs to consider */
		unsigned long min[4], avg[4], max[4];
		unsigned long long tot[4];

		for (k = 0; k < 4; k++) {
			max[k] = 0; avg[k] = 0; min[k] = ~0L;
		}

		for (j = 0; j < nloop; j++) {
			long diff;
			struct timespec ts0[4], ts1[4];

			if (send_and_stamp(sock[0], pkt, i, 0) < 0) {
				fprintf(stderr, "%s: send: %s\n", argv[0],
					strerror(errno));
				exit(1);
			}
			if (get_stamp(ts0) < 0) {
				fprintf(stderr, "%s: getstamp(send): %s\n",
					argv[0], strerror(errno));
				continue;
			}

			if (recv_and_stamp(sock[1], pkt, sizeof(pkt), 0) < 0) {
				fprintf(stderr, "%s: recv: %s\n", argv[0],
					strerror(errno));
				exit(1);
			}
			get_stamp(ts1);
			for (k = 0; 0 && k < 4; k++) {
				printf("%i: %10li.%09li %10li.%09li\n", k,
				       ts0[k].tv_sec, ts0[k].tv_nsec,
				       ts1[k].tv_sec, ts1[k].tv_nsec);
			}
			/* collect differences for all 4 values*/
			for (k = 0; k < 4; k++) {
				diff = tsdiff(ts1 + k, ts0 + k);
				tot[k] += diff;
				if (diff < min[k]) min[k] = diff;
				if (diff > max[k]) max[k] = diff;
				if (0)
					printf("%li%c", diff,
					       k == 3 ? '\n' : ' ');
			}
		}
		for (k = 0; k < 4; k++) {
			avg[k] = tot[k] / nloop;
			printf("size %4i  ts %i:  %7li %7li %7li\n", i, k,
			       min[k], avg[k], max[k]);
		}
	}
	exit(0);
}

