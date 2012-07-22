/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 */

#include <endpoint.h>
#include <ppsi/ppsi.h>
#include "dev/softpll_ng.h"
#include "spec.h"
#include "../proto-ext-whiterabbit/wr-constants.h"

#if 0
typedef struct {
  /* When non-zero: port state is valid */
  int valid;

  /* WR-PTP role of the port (Master, Slave, etc.) */
  int mode;

  /* TX and RX delays (combined, big Deltas from the link model in the spec) */
  uint32_t delta_tx;
  uint32_t delta_rx;

  /* DDMTD raw phase value in picoseconds */
  uint32_t phase_val;

  /* When non-zero: phase_val contains a valid phase readout */
  int phase_val_valid;

  /* When non-zero: link is up */
  int up;

  /* When non-zero: TX path is calibrated (delta_tx contains valid value) */
  int tx_calibrated;

  /* When non-zero: RX path is calibrated (delta_rx contains valid value) */
  int rx_calibrated;
  int tx_tstamp_counter;
  int rx_tstamp_counter;
  int is_locked;
  int lock_priority;

  // timestamp linearization paramaters

  uint32_t phase_setpoint; // DMPLL phase setpoint (picoseconds)

  uint32_t clock_period; // reference lock period in picoseconds
  uint32_t t2_phase_transition; // approximate DMTD phase value (on slave port) at which RX timestamp (T2) counter transistion occurs (picoseconds)

  uint32_t t4_phase_transition; // approximate phase value (on master port) at which RX timestamp (T4) counter transistion occurs (picoseconds)

  uint8_t hw_addr[6];
  int hw_index;
  int32_t fiber_fix_alpha;
} hexp_port_state_t;

extern int32_t cal_phase_transition;
extern int32_t sfp_alpha;

static int read_phase_val(hexp_port_state_t *state)
{
  int32_t dmtd_phase;

  if(spll_read_ptracker(0, &dmtd_phase, NULL))
  {
    state->phase_val = dmtd_phase;
    state->phase_val_valid = 1;
  }
  else
  {
    state->phase_val = 0;
    state->phase_val_valid = 0;
  }

  return 0;
}

static int halexp_get_port_state(hexp_port_state_t *state, const char *port_name)
{
  state->valid         = 1;
  /* FIXME Unused in the current context?
#ifdef WRPC_MASTER
  state->mode          = HEXP_PORT_MODE_WR_MASTER;
#else
  state->mode          = HEXP_PORT_MODE_WR_SLAVE;
#endif*/
  ep_get_deltas( &state->delta_tx, &state->delta_rx);
  read_phase_val(state);
  state->up            = ep_link_up(NULL);
  state->tx_calibrated = 1;
  state->rx_calibrated = 1;
  state->is_locked     = spll_check_lock(0);
  state->lock_priority = 0;
  spll_get_phase_shift(0, NULL, &state->phase_setpoint);
  state->clock_period  = 8000;
  state->t2_phase_transition = cal_phase_transition;
  state->t4_phase_transition = cal_phase_transition;
  get_mac_addr(state->hw_addr);
  state->hw_index      = 0;
  state->fiber_fix_alpha = sfp_alpha;

  return 0;
}

static int ptpd_netif_read_calibration_data(const char *ifaceName,
  uint64_t *deltaTx, uint64_t *deltaRx, int32_t *fix_alpha,
  int32_t *clock_period)
{
  hexp_port_state_t state;

  halexp_get_port_state(&state, ifaceName);

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

#endif
/* Begin of exported functions */

int spec_calibrating_disable(struct pp_instance *ppi, int txrx)
{
	return WR_HW_CALIB_OK;
}

int spec_calibrating_enable(struct pp_instance *ppi, int txrx)
{
	return WR_HW_CALIB_OK;
}

int spec_calibrating_poll(struct pp_instance *ppi, int txrx, uint64_t *delta)
{
	uint64_t delta_rx = 0, delta_tx = 0;

	/* FIXME: why delta is 64bit whereas ep_get_deltas accepts 32bit? */
	ep_get_deltas( &delta_tx, &delta_rx);

	if (txrx == WR_HW_CALIB_TX)
		*delta = delta_tx;
	else
		*delta = delta_rx;

	return WR_HW_CALIB_READY;
}

int spec_calibration_pattern_enable(struct pp_instance *ppi,
                                          unsigned int calibrationPeriod,
                                          unsigned int calibrationPattern,
                                          unsigned int calibrationPatternLen)
{
	ep_cal_pattern_enable();
	return WR_HW_CALIB_OK;
}

int spec_calibration_pattern_disable(struct pp_instance *ppi)
{
	ep_cal_pattern_disable();
	return WR_HW_CALIB_OK;
}

int wr_calibrating_disable(struct pp_instance *ppi, int txrx)
	__attribute__((alias("spec_calibrating_disable")));

int wr_calibrating_enable(struct pp_instance *ppi, int txrx)
	__attribute__((alias("spec_calibrating_enable")));

int wr_calibrating_poll(struct pp_instance *ppi, int txrx, uint64_t *delta)
	__attribute__((alias("spec_calibrating_poll")));

int wr_calibration_pattern_enable(struct pp_instance *ppi,
	unsigned int calibrationPeriod, unsigned int calibrationPattern,
	unsigned int calibrationPatternLen)
	__attribute__((alias("spec_calibration_pattern_enable")));
	
int wr_calibration_pattern_disable(struct pp_instance *ppi)
	__attribute__((alias("spec_calibration_pattern_disable")));
