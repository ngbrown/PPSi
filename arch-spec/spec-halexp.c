/*
 * Aurelio Colosimo for CERN, 2012 -- GNU LGPL v2.1 or later
 */
#include <ppsi/ppsi.h>
#include <softpll_ng.h>
#include "spec.h"

extern int32_t sfp_alpha;
extern int32_t sfp_deltaTx;
extern int32_t sfp_deltaRx;
extern uint32_t cal_phase_transition;

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

int halexp_get_port_state(hexp_port_state_t *state,
			  const char *port_name)
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
