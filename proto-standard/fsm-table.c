/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

/*
 * This is the default state machine table. Can be overrridden by an extended
 * protocol can define its own stuff. It is in its own source file, so
 * the linker can avoid pulling this data space if another table is there.
 */

struct pp_state_table_item pp_state_table[] = {
	{ PPS_INITIALIZING,	"initializing",	pp_initializing,},
	{ PPS_FAULTY,		"faulty",	pp_faulty,},
	{ PPS_DISABLED,		"disabled",	pp_disabled,},
	{ PPS_LISTENING,	"listening",	pp_listening,},
	{ PPS_PRE_MASTER,	"pre-master",	pp_pre_master,},
	{ PPS_MASTER,		"master",	pp_master,},
	{ PPS_PASSIVE,		"passive",	pp_passive,},
	{ PPS_UNCALIBRATED,	"uncalibrated",	pp_uncalibrated,},
	{ PPS_SLAVE,		"slave",	pp_slave,},
	{ PPS_END_OF_TABLE,}
};
