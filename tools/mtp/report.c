#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "mtp.h"

#define NSEC_PER_SEC (1000*1000*1000)

static signed long long tsdiff(struct timespec *new, struct timespec *old)
{
	return (new->tv_sec - old->tv_sec) * (long long)NSEC_PER_SEC
		+ new->tv_nsec - old->tv_nsec;
}

void mtp_result(struct mtp_packet *pkt)
{
	int i;
	long total, remote, rtt;
	signed long long delta; /* may be more than 32 bits (4 seconds) */

	/* Report information about the values that are not 0 */
	for (i = 0; i < 3; i++) {
		if (pkt->t[0][i].tv_sec == 0)
			continue;

		if (0) {
			int j;
			printf("Times at slot %i:\n", i);
			for (j = 0; j < 4; j++) {
				printf("   t%i: %li.%09li\n", i,
				       pkt->t[j][i].tv_sec,
				       pkt->t[j][i].tv_nsec);
			}
		}
		total = tsdiff(pkt->t[3] + i, pkt->t[0] + i);
		remote = tsdiff(pkt->t[2] + i, pkt->t[1] + i);
		rtt = total - remote;
		delta = tsdiff(pkt->t[1] + i, pkt->t[0] + i);
		if (0) {
			printf("total %li\n", total);
			printf("remote %li\n", remote);
			printf("rtt %li\n", rtt);
			printf("bigdelta %lli\n", delta);
		}
		delta -= rtt / 2;

		printf("%i: rtt %12.9lf   delta %13.9lf\n",
		       i, (double)rtt / NSEC_PER_SEC,
		       (double)delta / NSEC_PER_SEC);
	}
}
