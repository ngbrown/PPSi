/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "convert.h"

/*
 * This is a simple tools to slightly change the system time, by jumping
 * it forward or backward. Please see ./adjtime for a smooth update.
 */

int main(int argc, char **argv)
{
	struct timeval tv, otv;

	if (argc != 2) {
		fprintf(stderr, "%s: use \"%s <delta>\"\n"
			"   <delta> is a floating-point number of seconds\n",
			argv[0], argv[0]);
		exit(1);
	}

	if (str_to_tv(argv[1], &tv) < 0) {
		fprintf(stderr, "%s: not a time (float) number \"%s\"\n",
			argv[0], argv[1]);
		exit(1);
	}

	fprintf(stderr, "Requesting time-jump: %s seconds\n", tv_to_str(&tv));

	if (gettimeofday(&otv, NULL) < 0) {
		fprintf(stderr, "%s: gettimeofday(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	otv.tv_sec += tv.tv_sec;
	otv.tv_usec += tv.tv_usec;
	if (otv.tv_usec > 1000 * 1000) {
		otv.tv_usec -= 1000 * 1000;
		otv.tv_sec++;
	}
	if (settimeofday(&otv, NULL) < 0) {
		fprintf(stderr, "%s: settimeofday(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	exit(0);
}
