/*
 * There is no such thing as a timer in ptp. We only have timeouts.
 *
 * A timeout is a point in time. We calculate; we can be earlier; we
 * can be later. The only thing which is arch-specific is calculating.
 */
#include <ppsi/ppsi.h>
#include <time.h>

unsigned long pp_calc_timeout(int millisec)
{
	struct timespec now;
	uint64_t now_ms;
	unsigned long result;

	if (!millisec)
		millisec = 1;
	clock_gettime(CLOCK_MONOTONIC, &now);
	now_ms = 1000LL * now.tv_sec + now.tv_nsec / 1000 / 1000;

	result = now_ms + millisec;
	return result ? result : 1; /* cannot return 0 */
}
