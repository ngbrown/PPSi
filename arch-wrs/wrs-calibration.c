/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 */

#include <ppsi/ppsi.h>

#include "ppsi-wrs.h"

#define HAL_EXPORT_STRUCTURES
#include <hal_exports.h>

static int wrs_read_calibration_data(struct pp_instance *ppi,
  uint32_t *deltaTx, uint32_t *deltaRx, int32_t *fix_alpha,
  int32_t *clock_period)
{
	/* FIXME wrs_read_calibration_data */
	return 0;
}

int wrs_calibrating_disable(struct pp_instance *ppi, int txrx)
{
	return 0;
}

int wrs_calibrating_enable(struct pp_instance *ppi, int txrx)
{
	return 0;
}

int wrs_calibrating_poll(struct pp_instance *ppi, int txrx, uint32_t *delta)
{
	/* FIXME wrpc_calibrating_poll */
	return 0;
}

int wrs_calibration_pattern_enable(struct pp_instance *ppi,
				    unsigned int calibrationPeriod,
				    unsigned int calibrationPattern,
				    unsigned int calibrationPatternLen)
{
	/* FIXME wrs_calibration_pattern_enable */
	return 0;
}

int wrs_calibration_pattern_disable(struct pp_instance *ppi)
{
	/* FIXME wrpc_calibration_pattern_disable */
	return 0;
}

int wr_calibrating_disable(struct pp_instance *ppi, int txrx)
	__attribute__((alias("wrs_calibrating_disable")));

int wr_calibrating_enable(struct pp_instance *ppi, int txrx)
	__attribute__((alias("wrs_calibrating_enable")));

int wr_calibrating_poll(struct pp_instance *ppi, int txrx, uint32_t *delta)
	__attribute__((alias("wrs_calibrating_poll")));

int wr_calibration_pattern_enable(struct pp_instance *ppi,
	unsigned int calibrationPeriod, unsigned int calibrationPattern,
	unsigned int calibrationPatternLen)
	__attribute__((alias("wrs_calibration_pattern_enable")));

int wr_calibration_pattern_disable(struct pp_instance *ppi)
	__attribute__((alias("wrs_calibration_pattern_disable")));

int wr_read_calibration_data(struct pp_instance *ppi,
	uint32_t *deltaTx, uint32_t *deltaRx, int32_t *fix_alpha,
	int32_t *clock_period)
	__attribute__((alias("wrs_read_calibration_data")));
