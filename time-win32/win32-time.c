#include <ppsi/ppsi.h>

#include <Windows.h>

#define FILETIME_PER_SECOND ((ULONGLONG) 10000000LL)
#define NANOSECONDS_PER_FILETIME ((ULONGLONG) 100LL)
#define FILETIME_SEC_TO_UNIX_EPOCH 11644473600LL

static int win32_time_get(struct pp_instance *ppi, TimeInternal *t)
{
	FILETIME ft;
	ULONGLONG qwResult;
	
	// Use GetSystemTimePreciseAsFileTime on Windows 8 and above?
	GetSystemTimeAsFileTime(&ft);

	// Copy the time into a quadword. convert to seconds and nanoseconds
	qwResult = (((ULONGLONG)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
	Integer32 seconds = qwResult / FILETIME_PER_SECOND; // seconds since 1601-01-01T00:00:00Z
	Integer32 nanoseconds = (qwResult - (seconds * FILETIME_PER_SECOND)) * NANOSECONDS_PER_FILETIME;
	Integer32 unix_seconds = seconds - FILETIME_SEC_TO_UNIX_EPOCH;

	/* TAI = UTC + ~36 */
	t->seconds = unix_seconds + DSPRO(ppi)->currentUtcOffset;
	t->nanoseconds = nanoseconds;
	t->correct = 1;
	if (!(pp_global_d_flags & PP_FLAG_NOTIMELOG)) {
		// show in TAI? the recv stamp does. 
		pp_diag(ppi, time, 2, "%s: %9li.%09li\n", __FUNCTION__,
			unix_seconds, nanoseconds);
	}
	return 0;
}

static int win32_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	if (!t) { /* Change the network notion of the utc/tai offset */
		// TODO
		pp_diag(ppi, time, 1, "New TAI offset: %i\n",
			DSPRO(ppi)->currentUtcOffset);
		return 0;
	}

	// TODO

	/* UTC = TAI - ~36 */
	Integer32 seconds = t->seconds - DSPRO(ppi)->currentUtcOffset;
	Integer32 nanoseconds = t->nanoseconds;
	pp_diag(ppi, time, 1, "%s: %9li.%09li\n", __FUNCTION__,
		seconds, nanoseconds);
	return 0;
}

static DWORD systemTimeIncrement; // hardware clock counts (100-nanosecond units)

// return Frequency offset, in units of 1000 ppm (parts per million) - parts per billion?
//  positive or negative, not -1
static int win32_time_init_servo(struct pp_instance *ppi)
{
	DWORD timeAdjustment; // clock increment per systemTimeIncrement interval (100-nanosecond units)
	
	BOOL timeAdjustmentDisabled;

	GetSystemTimeAdjustment(&timeAdjustment, &systemTimeIncrement, &timeAdjustmentDisabled);

	if (timeAdjustmentDisabled) {
		timeAdjustment = systemTimeIncrement;
	}

	// 100-nanoseconds per systemTimeIncrement
	int adjustmentOffset = ((timeAdjustment >= systemTimeIncrement) 
		? (int)(timeAdjustment - systemTimeIncrement) 
		: -(int)(systemTimeIncrement - timeAdjustment));

	// ppb
	int freq_offset = (adjustmentOffset * 1000000000) / systemTimeIncrement; // 100-nanoseconds offset / 100-nanoseconds period * 1 billion

	pp_diag(ppi, time, 2, "%s\n", __FUNCTION__);
	return freq_offset;
}

static int win32_time_adjust(struct pp_instance *ppi, long offset_ns, long freq_ppb)
{
	if (freq_ppb) {
		if (freq_ppb > PP_ADJ_FREQ_MAX)
			freq_ppb = PP_ADJ_FREQ_MAX;
		if (freq_ppb < -PP_ADJ_FREQ_MAX)
			freq_ppb = -PP_ADJ_FREQ_MAX;
	}

	pp_diag(ppi, time, 1, "%s: %li %li\n", __FUNCTION__, offset_ns, freq_ppb);
	return 0;
}

static int win32_time_adjust_offset(struct pp_instance *ppi, long offset_ns)
{
	return win32_time_adjust(ppi, offset_ns, 0);
}

static int win32_time_adjust_freq(struct pp_instance *ppi, long freq_ppb)
{
	return win32_time_adjust(ppi, 0, freq_ppb);
}

static LARGE_INTEGER performanceFrequency = { 0, 0 }; // counts per second

static unsigned long win32_calc_timeout(struct pp_instance *ppi, int millisec)
{
	unsigned long result;
	LARGE_INTEGER performanceCount;
	uint64_t now_ms;

	if (!millisec) {
		millisec = 1;
	}

	if (!performanceFrequency.QuadPart) {
		QueryPerformanceFrequency(&performanceFrequency);
	}

	QueryPerformanceCounter(&performanceCount);

	now_ms = (performanceCount.QuadPart /*counts from system start*/ ) / ((performanceFrequency.QuadPart /*counts per second*/) / (1000 /*millisec per sec*/ )); // millisec from system start

	result = now_ms + millisec;
	return result ? result : 1; /* cannot return 0 */
}

struct pp_time_operations win32_time_ops = {
	.get = win32_time_get,
	.set = win32_time_set,
	.adjust = win32_time_adjust,
	.adjust_offset = win32_time_adjust_offset,
	.adjust_freq = win32_time_adjust_freq,
	.init_servo = win32_time_init_servo,
	.calc_timeout = win32_calc_timeout,
};
