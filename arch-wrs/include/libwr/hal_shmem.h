#ifndef __LIBWR_HAL_SHMEM_H__
#define __LIBWR_HAL_SHMEM_H__

#include <hal_exports.h>
#include <libwr/sfp_lib.h>

/* Port state machine states */
#define HAL_PORT_STATE_DISABLED 0
#define HAL_PORT_STATE_LINK_DOWN 1
#define HAL_PORT_STATE_UP 2
#define HAL_PORT_STATE_CALIBRATION 3
#define HAL_PORT_STATE_LOCKING 4

#define DEFAULT_T2_PHASE_TRANS 0
#define DEFAULT_T4_PHASE_TRANS 0

/* Port delay calibration parameters */
typedef struct hal_port_calibration {

	/* PHY delay measurement parameters for PHYs which require
	   external calibration (i.e. with the feedback network. */

	/* minimum possible delay introduced by the PHY. Expressed as time
	   (in picoseconds) between the beginning of the symbol on the serial input
	   and the rising edge of the RX clock at which the deserialized word is
	   available at the parallel output of the PHY. */
	uint32_t phy_rx_min;

	/* the same set of parameters, but for the TX path of the PHY */
	uint32_t phy_tx_min;

	/* Current PHY (clock-to-serial-symbol) TX and RX delays, in ps */
	uint32_t delta_tx_phy;
	uint32_t delta_rx_phy;

	/* Current board routing delays (between the DDMTD inputs to
	   the PHY clock inputs/outputs), in picoseconds */
	uint32_t delta_tx_board;
	uint32_t delta_rx_board;

	/* When non-zero: RX path is calibrated (delta_*_rx contain valid values) */
	int rx_calibrated;
	/* When non-zero: TX path is calibrated */
	int tx_calibrated;

	struct shw_sfp_caldata sfp;

} hal_port_calibration_t;

/* Internal port state structure */
struct hal_port_state {
	/* non-zero: allocated */
	int in_use;
	/* linux i/f name */
	char name[16];

	/* MAC addr */
	uint8_t hw_addr[6];

	/* ioctl() hw index */
	int hw_index;

	/* file descriptor for ioctls() */
	int fd;
	int hw_addr_auto;

	/* port timing mode (HEXP_PORT_MODE_xxxx) */
	int mode;

	/* port FSM state (HAL_PORT_STATE_xxxx) */
	int state;

	/* unused */
	int index;

	/* 1: PLL is locked to this port */
	int locked;

	/* calibration data */
	hal_port_calibration_t calib;

	/* current DMTD loopback phase (ps) and whether is it valid or not */
	uint32_t phase_val;
	int phase_val_valid;
	int tx_cal_pending, rx_cal_pending;
	/* locking FSM state */
	int lock_state;

	/* Endpoint's base address */
	uint32_t ep_base;
};

/* This is the overall structure stored in shared memory */
#define HAL_SHMEM_VERSION 1
struct hal_shmem_header {
	int nports;
	struct hal_port_state *ports;
};

/*
 * The following functions were in userspace/wrsw_hal/hal_ports.c,
 * and are used to marshall data for the RPC format. Now that we
 * offer shared memory, it is the caller who must convert data to
 * the expected format (which remains the RPC one as I write this).
 */
struct hal_port_state *hal_port_lookup(struct hal_port_state *ports,
				       const char *name);
int hal_port_query_ports(struct hexp_port_list *list,
			 const struct hal_port_state *ports);
int hal_port_get_exported_state(struct hexp_port_state *state,
				struct hal_port_state *ports,
				const char *port_name);

#endif /*  __LIBWR_HAL_SHMEM_H__ */
