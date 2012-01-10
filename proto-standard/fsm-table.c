/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

#include <pptp/pptp.h>

/*
 * This is the default state machine table. It is weak so an extension
 * protocol can define its own stuff. It is in its own source file, so
 * the linker can avoid pulling this data space if another table is there.
 */

struct pp_state_table_item pp_state_table[] = {
	{ PPS_INITIALIZING, pp_initializing,},
	{ PPS_FAULTY,	    pp_faulty,},
	{ PPS_DISABLED,     pp_disabled,},
	{ PPS_LISTENING,    pp_listening,},
	{ PPS_PRE_MASTER,   pp_pre_master,},
	{ PPS_MASTER,       pp_master,},
	{ PPS_PASSIVE,      pp_passive,},
	{ PPS_UNCALIBRATED, pp_uncalibrated,},
	{ PPS_SLAVE,        pp_slave,},
	{ PPS_END_OF_TABLE,}
};
