/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released to the public domain
 */

#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>
#include <hal_exports.h>
#include <wr-api.h>

extern ptpdexp_sync_state_t cur_servo_state;

/* minipc Encoding  of the supported commands */

#define PTPDEXP_COMMAND_TRACKING 1
#define PTPDEXP_COMMAND_MAN_ADJUST_PHASE 2

static struct minipc_pd __rpcdef_get_sync_state = {
	.name = "get_sync_state",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, ptpdexp_sync_state_t),
	.args = {
			MINIPC_ARG_END,
	},
};

static struct minipc_pd __rpcdef_cmd = {
	.name = "cmd",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
			MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
			MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
			MINIPC_ARG_END,
	},
};

/* Fill struct ptpdexp_sync_state_t with current servo state */
static int wrsipc_get_sync_state(ptpdexp_sync_state_t *state)
{
	if (!BUILT_WITH_WHITERABBIT)
		cur_servo_state.valid = 0; /* unneeded, likely */
	memcpy(state, &cur_servo_state, sizeof(*state));
	return 0;
}

/* Execute command coming ipc */
static int wrsipc_cmd(int cmd, int value)
{
	if (BUILT_WITH_WHITERABBIT) {
		if(cmd == PTPDEXP_COMMAND_TRACKING)
			wr_servo_enable_tracking(value);

		if(cmd == PTPDEXP_COMMAND_MAN_ADJUST_PHASE)
			wr_servo_man_adjust_phase(value);
	}
	return 0;

}

/* Two functions to manage packet/args conversions */
static int export_get_sync_state(const struct minipc_pd *pd,
				 uint32_t *args, void *ret)
{
	ptpdexp_sync_state_t state;

	wrsipc_get_sync_state(&state);

	*(ptpdexp_sync_state_t *)ret = state;
	return 0;

}

static int export_cmd(const struct minipc_pd *pd,
				 uint32_t *args, void *ret)
{
	int i;
	i = wrsipc_cmd(args[0], args[1]);
	*(int *)ret = i;
	return 0;
}

/* To be called at startup, right after the creation of server channel */
void wrs_init_ipcserver(struct minipc_ch *ppsi_ch)
{
	__rpcdef_get_sync_state.f = export_get_sync_state;
	__rpcdef_cmd.f = export_cmd;

	minipc_export(ppsi_ch, &__rpcdef_get_sync_state);
	minipc_export(ppsi_ch, &__rpcdef_cmd);
}
