/*
 * There is no such thing as a timer in ptp. We only have timeouts.
 *
 * A timeout is a point in time. We calculate; we can be earlier; we
 * can be later. The only thing which is arch-specific is calculating.
 */
#include <ppsi/ppsi.h>
#include <syscon.h>

unsigned long pp_calc_timeout(int millisec)
{
	unsigned long result;

	if (!millisec)
		millisec = 1;
	result = timer_get_tics() + millisec;
	return result ? result : 1; /* cannot return 0 */
}
