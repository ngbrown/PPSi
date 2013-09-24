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
#include <time.h>
#include <sys/time.h>
#include <sys/timex.h>

/*
 * This is a simple tool to check/change the rate, for testing.
 * The numeric argument is (ppm * 1000) << 16.
 */
char *retvals[] = {"OK", "Insert leap", "Delete leap", "Leap in progress",
		   "Leap occurred", "Clock not synchnonized"};

int main(int argc, char **argv)
{
	struct timex t;
	double f;
	long l;
	char c;

	if ((argc > 1 && argv[1][0] == '-')
	    || -argc > 3
	    || (argc == 3 && strcmp(argv[2], "ppm"))) {
		fprintf(stderr, "%s: use \"%s [<adj-value> [ppm]]\"\n",
			argv[0], argv[0]);
		exit(1);
	}

	memset(&t, 0, sizeof(t));
	adjtimex(&t);
	if (argc == 1) {
		f = t.freq / 65536.0;
		printf("rate: %li (%.06f ppm)\n", t.freq, f);
		exit(0);
	}

	if (argc == 3) { /* "ppm": convert as float */
		if (sscanf(argv[1], "%lf%c", &f, &c) != 1) {
			fprintf(stderr, "%s: not a float number \"%s\"\n",
				argv[0], argv[1]);
			exit(1);
		}
		l = f * 65536;

	} else {
		if (sscanf(argv[1], "%li%c", &l, &c) != 1) {
			fprintf(stderr, "%s: not an integer number \"%s\"\n",
				argv[0], argv[1]);
			exit(1);
		}
	}

	/* to set the sate we must set the pll flag too */
	t.modes = 0;
	if (!t.status &STA_PLL) {
		fprintf(stderr, "%s: adding STA_PLL to adjtimex.status\n",
			argv[0]);
		t.modes |= MOD_STATUS;
		t.status |= STA_PLL;
	}
	t.modes |= MOD_FREQUENCY;
	t.freq = l;
	l = adjtimex(&t);
	if (l != 0) {
		fprintf(stderr, "%s: adjtimex(rate=%li): %li (%s)\n",
			argv[0], t.freq, l,
			l < 0 ? strerror(errno) : retvals[l]);
	}
	exit(0);
}
