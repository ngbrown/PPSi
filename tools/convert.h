/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

/*
 * Conversions are boring stuff: we can't use floats or doubles (because
 * 0.3 becomes 0.29999) and we need to ensure the fraction is positive.
 */

static inline int str_to_tv(char *s, struct timeval *tv)
{
	int i, sign = 1, micro = 0, sec = 0;
	char *smicro = NULL;
	char c;

	if (strlen(s) > 32)
		return -1;
	if (s[0] == '-') {
		sign = -1;
		s++;
	}
	if (sscanf(s, "%d.%d%c", &sec, &micro, &c) > 2)
		return -1;
	smicro = strchr(s, '.');
	if (smicro) {
		smicro++;
		if (sscanf(smicro, "%d%c", &micro, &c) != 1)
			return -1;
	}

	/* now we have sign, sec, and smicro (string of microseconds) */
	if (smicro) {
		/*  check how long it is and scale*/
		i = strlen(smicro);
		if (i > 6)
			return -1;
		while (i < 6) {
			micro *= 10;
			i++;
		}
	}

	tv->tv_sec = sec * sign;
	tv->tv_usec = micro;
	if (sign < 0 && micro) { /* "-1.2" ->  -2 + .8 */
		tv->tv_sec--;
		tv->tv_usec = 1000 * 1000 - micro;
	}
	return 0;
}

/* returns static storage */
static inline char *tv_to_str(struct timeval *origtv)
{
	static char res[32];
	struct timeval localtv = *origtv;
	struct timeval *tv = &localtv;
	char *s = res;

	if (tv->tv_usec < 0) {
		tv->tv_sec--;
		tv->tv_usec += 1000 * 1000;
	}
	if (tv->tv_usec < 0 || tv->tv_usec > 1000 * 1000)
		return "(error)";
	if (tv->tv_sec < 0) {
		*(s++) = '-';
		if (tv->tv_usec) { /* -2 + .8 --> "-1.2" */
			tv->tv_sec++;
			tv->tv_usec = 1000 * 1000 - tv->tv_usec;
		}
		tv->tv_sec *= -1;
	}
	sprintf(s, "%d.%06d", (int)tv->tv_sec, (int)tv->tv_usec);
	return res;
}
