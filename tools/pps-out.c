/*
 * Trivial tool to output a software-generated pps pulse.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <sys/time.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
	struct timeval tv, tv2;
	unsigned long sec;
	struct sched_param param;
	int port, sfd, mask, m1, diff, d2;
	char c;

	if (argc != 2) {
		fprintf(stderr, "%s: use \"%s <hex-port-or-/dev/serial>\" \n",
			argv[0], argv[0]);
		exit(1);
	}

	if (sscanf(argv[1], "%x%c", &port, &c) == 1) {
		sfd = 0;
		if (ioperm(port, 1, 1) < 0) {
			fprintf(stderr, "%s: ioperm(0x%x): %s\n",
				argv[0], port,
				strerror(errno));
			exit(1);
		}
	} else if (argv[1][0] != '/') {
		fprintf(stderr, "%s: not a hex number nor a device\"%s\"\n",
			argv[0], argv[1]);
		exit(1);
	} else {
		sfd = open(argv[1], O_RDWR);
		if (sfd < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1],
				strerror(errno));
			exit(1);
		}
		if (ioctl(sfd, TIOCMGET, &mask) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1],
				strerror(errno));
			exit(1);
		}
		mask &= ~(TIOCM_RTS | TIOCM_DTR);
		m1 = mask | TIOCM_RTS | TIOCM_DTR;
	}

	/* Get high priority, but continue anyways on failure */
	param.sched_priority = 10;
	if (sched_setscheduler(0, SCHED_FIFO, &param)<0) {
		fprintf(stderr, "%s: setscheduler: %s\n",
			argv[0], strerror(errno));
	}

	/* Check how much we take, doing things 10 times */
	gettimeofday(&tv, NULL);
	gettimeofday(&tv2, NULL); gettimeofday(&tv2, NULL);
	gettimeofday(&tv2, NULL); gettimeofday(&tv2, NULL);
	gettimeofday(&tv2, NULL);
	gettimeofday(&tv2, NULL); gettimeofday(&tv2, NULL);
	gettimeofday(&tv2, NULL); gettimeofday(&tv2, NULL);
	gettimeofday(&tv2, NULL);
	diff = tv2.tv_usec - tv.tv_usec;
	if (diff < 0)
		diff += 1000 * 1000;
	if (getenv("VERBOSE"))
		printf("gettimeofday takes %i.%i usec\n", diff / 10, diff % 10);

	gettimeofday(&tv, NULL);
	usleep(0); usleep(0); usleep(0); usleep(0); usleep(0);
	usleep(0); usleep(0); usleep(0); usleep(0); usleep(0);
	gettimeofday(&tv2, NULL);
	d2 = diff;
	diff = tv2.tv_usec - tv.tv_usec;
	if (diff < 0)
		diff += 1000 * 1000;
	diff -= (d2 + 5) / 10;
	if (getenv("VERBOSE"))
		printf("udelay takes %i.%i usec\n", diff / 10, diff % 10);

	/* prepare */
	if (sfd)
		ioctl(sfd, TIOCMSET, &mask);
	else
		outb(0x00, port);
	gettimeofday(&tv, NULL);
	sec = tv.tv_sec;

	/* run */
	while (1) {
		sec++;
		while (gettimeofday(&tv, NULL), tv.tv_sec < sec)
			if (tv.tv_usec < 990 * 1000)
				usleep((1000 * 1000 - tv.tv_usec) / 2);

		if (sfd)
			ioctl(sfd, TIOCMSET, &m1);
		else
			outb(0xff, port);
		gettimeofday(&tv2, NULL);
		if (getenv("VERBOSE"))
			printf("delayed between %3i and %3i usecs\n",
			       (int)tv.tv_usec, (int)tv2.tv_usec);
		usleep(50 * 1000);
		if (sfd)
			ioctl(sfd, TIOCMSET, &mask);
		else
			outb(0x00, port);
	}
}
