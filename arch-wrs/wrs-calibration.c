/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 */

#include <ppsi/ppsi.h>

#include <ppsi-wrs.h>

#define HAL_EXPORT_STRUCTURES
#include <hal_exports.h>

static int wrs_read_calibration_data(struct pp_instance *ppi,
	uint32_t *delta_tx, uint32_t *delta_rx, int32_t *fix_alpha,
	int32_t *clock_period)
{
	hexp_port_state_t state;

	minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_get_port_state,
		  &state, ppi->iface_name);

	if(state.valid && state.tx_calibrated && state.rx_calibrated) {

		pp_diag(ppi, servo, 1, "deltas: tx=%d, rx=%d)\n",
			state.delta_tx ,state.delta_rx);

		if(delta_tx)
			*delta_tx = state.delta_tx;

		if(delta_rx)
			*delta_rx = state.delta_rx;

		if(fix_alpha)
			*fix_alpha = state.fiber_fix_alpha;

		if(clock_period)
			*clock_period = state.clock_period;

		return WR_HW_CALIB_OK;
	}

	return WR_HW_CALIB_NOT_FOUND;
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
	uint32_t delta_tx, delta_rx;

	wrs_read_calibration_data(ppi, &delta_tx, &delta_rx, NULL, NULL);

	*delta = (txrx == WR_HW_CALIB_TX) ? delta_tx : delta_rx;

	return WR_HW_CALIB_READY;
}

int wrs_calibration_pattern_enable(struct pp_instance *ppi,
				    unsigned int calib_period,
				    unsigned int calib_pattern,
				    unsigned int calib_pattern_len)
{
	return WR_HW_CALIB_OK;
}

int wrs_calibration_pattern_disable(struct pp_instance *ppi)
{
	return WR_HW_CALIB_OK;
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
