/* CERN 2013 (author A. Rubini), part of the ppsi project, GPL v2 or later */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "convert.h"

/*
 * This is a simple tools to slightly change the system time, in order
 * to check when (and how) PTP resynchronizes it
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

	fprintf(stderr, "Requesting adjustment: %s seconds\n", tv_to_str(&tv));
	if (adjtime(&tv, &otv) < 0) {
		fprintf(stderr, "%s: adjtime(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	fprintf(stderr, "Previous adjustment: %s seconds\n", tv_to_str(&otv));
	exit(0);
}
