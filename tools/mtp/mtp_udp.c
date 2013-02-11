#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "mtp.h"

#define MTP_PORT 0x6d74  /* 'm' 't' */

static void run_passive_host(int argc, char **argv, int sock)
{
	int i;
	socklen_t slen;
	struct sockaddr_in addr;
	struct timeval tv1, tv2;
	struct mtp_packet pkt;

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
	if (pkt.ptype != MTP_FORWARD) {
		fprintf(stderr, "%s: wrong packet 1\n", argv[0]);
		exit(1);
	}

	/* send backward packet */
	pkt.ptype = MTP_BACKWARD;
	gettimeofday(&tv2, NULL);
	sendto(sock, &pkt, sizeof(pkt), 0,
	       (struct sockaddr *)&addr, sizeof(addr));

	/* send stamps */
	tv_to_ts(&tv1, pkt.t[1]);
	tv_to_ts(&tv2, pkt.t[2]);
	pkt.ptype = MTP_BACKSTAMP;
	sendto(sock, &pkt, sizeof(pkt), 0,
	       (struct sockaddr *)&addr, sizeof(addr));
}

static int run_active_host(int argc, char **argv, int sock,
			    struct sockaddr_in *addr)
{
	int i, tragic;
	socklen_t slen;
	struct hostent *h;
	struct timeval tv0, tv3;
	struct mtp_packet pkt;

	/* retrieve the remote host */
	h = gethostbyname(argv[1]);
	if (!h) {
		fprintf(stderr, "%s: %s: can't resolve hostname\n",
			argv[0], argv[1]);
		exit(1);
	}
	addr->sin_addr.s_addr = *(uint32_t *)h->h_addr;

	/* stamp and send the first packet */
	memset(&pkt, 0, sizeof(pkt));
	pkt.ptype = MTP_FORWARD;
	srand(time(NULL));
	pkt.tragic = tragic = rand();
	gettimeofday(&tv0, NULL);
	sendto(sock, &pkt, sizeof(pkt), 0,
	       (struct sockaddr *)addr, sizeof(*addr));

	/* get the second packet -- stamp and discard it */
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
	if (pkt.ptype != MTP_BACKWARD || pkt.tragic != tragic) {
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
	if (pkt.ptype != MTP_BACKSTAMP || pkt.tragic != tragic) {
		fprintf(stderr, "%s: wrong packet 2\n", argv[0]);
		exit(1);
	}

	/* add our stamp, we are using only the first value */
	tv_to_ts(&tv0, pkt.t[0]);
	tv_to_ts(&tv3, pkt.t[3]);

	mtp_result(&pkt);
	return 0;
}

int main(int argc, char **argv)
{
	int sock, mtp_listen = 0;
	struct sockaddr_in addr;

	if (argc != 2) {
		fprintf(stderr, "%s: Use: \"%s -l\" or \"%s <host>\"\n",
			argv[0], argv[0], argv[0]);
		exit(1);
	}
	if (!strcmp(argv[1], "-l")) {
		mtp_listen = 1;
	}

	/* open file descriptor */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		fprintf(stderr, "%s: socket(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	/* bind to a port, so we can get data  */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(MTP_PORT);
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "%s: bind(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	if (!mtp_listen)
		return run_active_host(argc, argv, sock, &addr);

	while (1)
		run_passive_host(argc, argv, sock);
}

