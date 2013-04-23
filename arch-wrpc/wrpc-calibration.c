/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 */

#define __REGS_H
#include <endpoint.h>
#include <ppsi/ppsi.h>
#include <softpll_ng.h>
#include <hal_exports.h>
#include "wrpc.h"
#include "../proto-ext-whiterabbit/wr-constants.h"

static int wrpc_read_calibration_data(struct pp_instance *ppi,
  uint32_t *deltaTx, uint32_t *deltaRx, int32_t *fix_alpha,
  int32_t *clock_period)
{
  hexp_port_state_t state;

  halexp_get_port_state(&state, ppi->iface_name);

  // check if the data is available
  if(state.valid)
  {

    if(fix_alpha)
      *fix_alpha = state.fiber_fix_alpha;

    if(clock_period)
      *clock_period = state.clock_period;

    //check if tx is calibrated,
    // if so read data
    if(state.tx_calibrated)
    {
      if(deltaTx) *deltaTx = state.delta_tx;
    }
    else
      return WR_HW_CALIB_NOT_FOUND;

    //check if rx is calibrated,
    // if so read data
    if(state.rx_calibrated)
    {
      if(deltaRx) *deltaRx = state.delta_rx;
    }
    else
      return WR_HW_CALIB_NOT_FOUND;

  }
  return WR_HW_CALIB_OK;

}


/* Begin of exported functions */

int wrpc_calibrating_disable(struct pp_instance *ppi, int txrx)
{
	return WR_HW_CALIB_OK;
}

int wrpc_calibrating_enable(struct pp_instance *ppi, int txrx)
{
	return WR_HW_CALIB_OK;
}

int wrpc_calibrating_poll(struct pp_instance *ppi, int txrx, uint32_t *delta)
{
	uint32_t delta_rx = 0, delta_tx = 0;

	/* FIXME: why delta was 64bit whereas ep_get_deltas accepts 32bit? */
	wrpc_read_calibration_data(ppi, &delta_tx, &delta_rx, NULL, NULL);

	if (txrx == WR_HW_CALIB_TX)
		*delta = delta_tx;
	else
		*delta = delta_rx;

	return WR_HW_CALIB_READY;
}

int wrpc_calibration_pattern_enable(struct pp_instance *ppi,
				    unsigned int calibrationPeriod,
				    unsigned int calibrationPattern,
				    unsigned int calibrationPatternLen)
{
	ep_cal_pattern_enable();
	return WR_HW_CALIB_OK;
}

int wrpc_calibration_pattern_disable(struct pp_instance *ppi)
{
	ep_cal_pattern_disable();
	return WR_HW_CALIB_OK;
}

int wr_calibrating_disable(struct pp_instance *ppi, int txrx)
	__attribute__((alias("wrpc_calibrating_disable")));

int wr_calibrating_enable(struct pp_instance *ppi, int txrx)
	__attribute__((alias("wrpc_calibrating_enable")));

int wr_calibrating_poll(struct pp_instance *ppi, int txrx, uint32_t *delta)
	__attribute__((alias("wrpc_calibrating_poll")));

int wr_calibration_pattern_enable(struct pp_instance *ppi,
	unsigned int calibrationPeriod, unsigned int calibrationPattern,
	unsigned int calibrationPatternLen)
	__attribute__((alias("wrpc_calibration_pattern_enable")));

int wr_calibration_pattern_disable(struct pp_instance *ppi)
	__attribute__((alias("wrpc_calibration_pattern_disable")));

int wr_read_calibration_data(struct pp_instance *ppi,
	uint32_t *deltaTx, uint32_t *deltaRx, int32_t *fix_alpha,
	int32_t *clock_period)
	__attribute__((alias("wrpc_read_calibration_data")));
