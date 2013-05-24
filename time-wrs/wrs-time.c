/*
 * Aurelio Colosimo for CERN, 2013 -- public domain
 */

#include <ppsi/ppsi.h>
#include <unix-time.h>

struct pp_time_operations wrs_time_ops = {
	.get = unix_time_get,
	.set = unix_time_set,
	.adjust = unix_time_adjust,
	.adjust_offset = unix_time_adjust_offset,
	.adjust_freq = unix_time_adjust_freq,
	.calc_timeout = unix_calc_timeout,
};
