


#include <ppsi/ppsi.h>

static int win32_time_get(struct pp_instance *ppi, TimeInternal *t)
{
}

static int win32_time_set(struct pp_instance *ppi, TimeInternal *t)
{
	return 0;
}

static int win32_time_adjust(struct pp_instance *ppi, long offset_ns, long freq_ppb)
{
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

static int win32_time_init_servo(struct pp_instance *ppi)
{
	return -1;
}

static unsigned long win32_calc_timeout(struct pp_instance *ppi, int millisec)
{
	return 1;
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
