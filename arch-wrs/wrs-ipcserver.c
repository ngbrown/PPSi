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

/* minipc Encoding  of the supported commands */

#define PTPDEXP_COMMAND_TRACKING 1
#define PTPDEXP_COMMAND_MAN_ADJUST_PHASE 2

static struct minipc_pd __rpcdef_cmd = {
	.name = "cmd",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
			MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
			MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
			MINIPC_ARG_END,
	},
};

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
	__rpcdef_cmd.f = export_cmd;

	minipc_export(ppsi_ch, &__rpcdef_cmd);
}
