/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

#define CMD_LINE_SEPARATOR {"", ""}

struct cmd_line_opt {
	char *flag;
	char *help;
};
static struct cmd_line_opt cmd_line_list[] = {
	{"-?", "show this page"},
	/* FIXME cmdline check if useful */
	//{"-c", "run in command line (non-daemon) mode"},
	//{"-S", "send output to syslog"},
	//{"-T", "set multicast time to live"},
	//{"-d", "display stats"},
	//{"-D", "display stats in .csv format"},
	//{"-R", "record data about sync packets in a file"},
	{"-C CONFIG_ITEM", "set configuration options as stated in CONFIG_ITEM\n\t"
		"CONFIG_ITEM must be a valid config string, enclosed by \" \""},
	{"-f FILE", "read configuration file"},
	{"-d STRING", "diagnostic level (see diag-macros.h)"},
	CMD_LINE_SEPARATOR,
	{"-x", "do not reset the clock if off by more than one second"},
	{"-O NUMBER", "do not reset the clock if offset is more than NUMBER nanoseconds"},
	{"-M NUMBER", "do not accept delay values of more than NUMBER nanoseconds"},
	{"-t", "do not adjust the system clock"},
	{"-a NUMBER,NUMBER", "specify clock servo P and I values (min == 1)"},
	{"-w NUMBER", "specify meanPathDelay filter stiffness"},
	CMD_LINE_SEPARATOR,
	{"-b NAME", "bind PTP to network interface NAME"},
	//{"-u ADDRESS", "also send uni-cast to ADDRESS\n"}, -- FIXME: useful?
	{"-e", "run in ethernet mode (level2)"},
	/* {"-h", "run in End to End mode"}, -- we only support end-to-end */
	/* {"-G", "run in gPTP mode (implies -e)"}, -- no peer-to-peer mode */
	{"-l NUMBER,NUMBER", "specify inbound, outbound latency in nsec"},
	CMD_LINE_SEPARATOR,
	{"-i NUMBER", "specify PTP domain number"},
	CMD_LINE_SEPARATOR,
	{"-n NUMBER", "specify announce interval in 2^NUMBER sec"},
	{"-y NUMBER", "specify sync interval in 2^NUMBER sec"},
	CMD_LINE_SEPARATOR,
	{"-g", "run as slave only"},
	{"-v NUMBER", "specify system clock allen variance"},
	{"-r NUMBER", "specify system clock accuracy"},
	{"-s NUMBER", "specify system clock class"},
	{"-p NUMBER", "specify priority1 attribute"},
	{"-q NUMBER", "specify priority2 attribute"},
	CMD_LINE_SEPARATOR,
	{NULL, NULL}
};

static void cmd_line_print_help(void)
{
	int i = 0;
	pp_printf("\nUsage: ppsi [OPTION]\n\n");

	while (1) {
		struct cmd_line_opt *o = &cmd_line_list[i];
		if (o->flag == NULL)
			break;
		pp_printf("%s\n\t%s\n", o->flag, o->help);
		i++;
	}
}

static void cmd_line_parse_two(char *a, int *n1, int *n2)
{
	int i, comma = 0;
	*n1 = *n2 = 0;
	for (i = 0; a[i] != '\0'; i++) {
		if (a[i] == ',') {
			comma = i;
			a[i] = '\0';
			*n1 = atoi(a);
			break;
		}
	}
	*n2 = atoi(&a[comma+1]);
	a[comma] = ',';
}

int pp_parse_cmdline(struct pp_globals *ppg, int argc, char **argv)
{
	int i, err = 0;
	int j;
	char *a; /* cmd line argument */
	int n1, n2; /* used by cmd_line_parse_two */

	for (i = 1; i < argc; i++) {
		a = argv[i];
		if (a[0] != '-')
			err = 1;
		else if (a[1] == '?')
			err = 1;
		else if (a[1] && a[2])
			err = 1;
		if (err) {
			cmd_line_print_help();
			return -1;
		}

		switch (a[1]) {
		case 'd':
			/* Use the general flags, per-instance TBD */
			a = argv[++i];
			pp_global_d_flags = pp_diag_parse(a);
			break;
		case 'C':
			if (pp_config_string(ppg, argv[++i]) != 0)
				return -1;
			break;
		case 'f':
			if (pp_config_file(ppg, 1, argv[++i]) != 0)
				return -1;
			break;
		case 'x':
			GOPTS(ppg)->flags |= PP_FLAG_NO_RESET;
			break;
		case 'O':
			a = argv[++i];
			GOPTS(ppg)->max_rst = atoi(a);
			if (GOPTS(ppg)->max_rst > PP_NSEC_PER_SEC) {
				pp_printf("Use -x to prevent jumps of"
				" more than one second\n");
				return -1;
			}
			break;
		case 'M':
			a = argv[++i];
			GOPTS(ppg)->max_dly = atoi(a);
			if (GOPTS(ppg)->max_dly > PP_NSEC_PER_SEC) {
				pp_printf("Use -x to prevent jumps of"
					" more than one second\n");
				return -1;
			}
			break;
		case 't':
			GOPTS(ppg)->flags |= PP_FLAG_NO_ADJUST;
			break;
		case 'a':
			a = argv[++i];
			cmd_line_parse_two(a, &n1, &n2);
			/* no negative or zero attenuation */
			if (n1 < 1 || n2 < 1)
				return -1;
			GOPTS(ppg)->ap = n1;
			GOPTS(ppg)->ai = n2;
			break;
		case 'w':
			a = argv[++i];
			GOPTS(ppg)->s = atoi(a);
			break;
		case 'b':
			a = argv[++i];
			if (ppg->nlinks == 1) {
				INST(ppg, 0)->iface_name = a;
				INST(ppg, 0)->port_name = a;
			} else {
				/* If ppsi.conf exists and more than one link is
				 * configured, it makes no sense trying to set an iface
				 * name */
				pp_printf("Can not use -b option in multi-link conf");
				return -1;
			}
			break;
		case 'l':
			a = argv[++i];
			cmd_line_parse_two(a, &n1, &n2);
			GOPTS(ppg)->inbound_latency.nanoseconds = n1;
			GOPTS(ppg)->outbound_latency.nanoseconds = n2;
			break;
		case 'i':
			a = argv[++i];
			GOPTS(ppg)->domain_number = atoi(a);
			break;
		case 'y':
			a = argv[++i];
			GOPTS(ppg)->sync_intvl = atoi(a);
			break;
		case 'n':
			a = argv[++i];
			/* Page 237 says 0 to 4 (1s .. 16s) */
			GOPTS(ppg)->announce_intvl = atoi(a);
			if (GOPTS(ppg)->announce_intvl < 0)
				GOPTS(ppg)->announce_intvl = 0;
			if (GOPTS(ppg)->announce_intvl > 4)
				GOPTS(ppg)->announce_intvl = 4;
			break;
		case 'g':
			GOPTS(ppg)->clock_quality.clockClass
				= PP_CLASS_SLAVE_ONLY;
			/* Apply -g option globally, to each configured link */
			for (j = 0; j < ppg->nlinks; j++)
				INST(ppg, j)->role = PPSI_ROLE_SLAVE;
			break;
		case 'v':
			a = argv[++i];
			GOPTS(ppg)->clock_quality.
				offsetScaledLogVariance = atoi(a);
			break;
		case 'r':
			a = argv[++i];
			GOPTS(ppg)->clock_quality.clockAccuracy =
				atoi(a);
			break;
		case 's':
			a = argv[++i];
			GOPTS(ppg)->clock_quality.clockClass =
				atoi(a);
			break;
		case 'p':
			a = argv[++i];
			GOPTS(ppg)->prio1 = atoi(a);
			break;
		case 'q':
			a = argv[++i];
			GOPTS(ppg)->prio2 = atoi(a);
			break;
		case 'h':
			/* ignored: was "GOPTS(ppg)->e2e_mode = 1;" */
			break;
		case 'e':
			/* Apply -e option globally, to each configured link */
			for (j = 0; j < ppg->nlinks; j++)
				INST(ppg, j)->proto = PPSI_PROTO_RAW;
			break;
		case 'G':
			/* gptp_mode not supported: fall through */
		default:
			cmd_line_print_help();
			return -1;
		}
	 }
	return 0;
}
