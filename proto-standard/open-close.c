/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

#include <pptp/pptp.h>
#include <pptp/diag.h>

/*
 * This file deals with opening and closing an instance. The channel
 * must already have been created. In practices, this initializes the
 * state machine to the first state.
 *
 * A protocol extension can override none or both of these functions.
 */

struct pp_runtime_opts default_rt_opts = {
	.no_adjust = TRUE,
	.no_rst_clk = PP_DEFAULT_NO_RESET_CLOCK,
	.ap = PP_DEFAULT_AP,
	.ai = PP_DEFAULT_AI,
	.s = PP_DEFAULT_DELAY_S,
	.max_foreign_records = PP_DEFAULT_MAX_FOREIGN_RECORDS,
	.cur_utc_ofst = PP_DEFAULT_UTC_OFFSET,
	.announce_intvl = PP_DEFAULT_ANNOUNCE_INTERVAL,
	.sync_intvl = PP_DEFAULT_SYNC_INTERVAL,
	.prio1 = PP_DEFAULT_PRIORITY1,
	.prio2 = PP_DEFAULT_PRIORITY2,
	.domain_number = PP_DEFAULT_DOMAIN_NUMBER,
	.ttl = 1,
};

struct cmd_line_opt {
	char *flag;
	char *help;
};

#define CMD_LINE_SEPARATOR {"", ""}

static struct cmd_line_opt cmd_line_list[] = {
	{"-?", "show this page"},
	/* FIXME cmdline check if useful
	{"-c", "run in command line (non-daemon) mode"},
	{"-f FILE", "send output to FILE"},
	{"-S", "send output to syslog"},
	{"-T", "set multicast time to live"},
	{"-d", "display stats"},
	{"-D", "display stats in .csv format"},
	{"-R", "record data about sync packets in a file"},
	*/
	{"-V", "run in verbose mode"},
	CMD_LINE_SEPARATOR,
	{"-x", "do not reset the clock if off by more than one second"},
	{"-O NUMBER", "do not reset the clock if offset is more than NUMBER nanoseconds"},
	{"-M NUMBER", "do not accept delay values of more than NUMBER nanoseconds"},
	{"-t", "do not adjust the system clock"},
	{"-a NUMBER,NUMBER", "specify clock servo P and I attenuations"},
	{"-w NUMBER", "specify one way delay filter stiffness"},
	CMD_LINE_SEPARATOR,
	{"-b NAME", "bind PTP to network interface NAME"},
	/* FIXME cmdline check if useful
	{"-u ADDRESS", "also send uni-cast to ADDRESS\n"},
	*/
	/* FIXME cmdline now choosen by #define
	{"-e", "run in ethernet mode (level2)"},
	*/
	{"-h", "run in End to End mode"},
	{"-l NUMBER,NUMBER", "specify inbound, outbound latency in nsec"},
	CMD_LINE_SEPARATOR,
	{"-o NUMBER", "specify current UTC offset"},
	{"-i NUMBER", "specify PTP domain number"},
	CMD_LINE_SEPARATOR,
	{"-n NUMBER", "specify announce interval in 2^NUMBER sec"},
	{"-y NUMBER", "specify sync interval in 2^NUMBER sec"},
	{"-m NUMBER", "specify max number of foreign master records"},
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
	pp_printf("\nUsage: pptp [OPTION]\n\n");

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

int pp_parse_cmdline(struct pp_instance *ppi, int argc, char **argv)
{
	int i;
	char *a; /* cmd line argument */
	int n1, n2; /* used by cmd_line_parse_two */

	for (i = 0; i < argc; i++) {
		a = argv[i];
		if ((a[0] == '-') && (a[1] != '\0')) {
			switch (a[1]) {
			case '?':
				cmd_line_print_help();
				return 1;
			case 'V':
				pp_diag_verbosity = 1;
				break;
			case 'x':
				OPTS(ppi)->no_rst_clk = 1;
				break;
			case 'O':
				a = argv[++i];
				OPTS(ppi)->max_rst = atoi(a);
				if(OPTS(ppi)->max_rst < PP_NSEC_PER_SEC) {
					pp_printf("Use -x to prevent jumps of"
					" more than one second\n");
					return -1;
				}
				break;
			case 'M':
				a = argv[++i];
				OPTS(ppi)->max_dly = atoi(a);
				if(OPTS(ppi)->max_dly < PP_NSEC_PER_SEC) {
					pp_printf("Use -x to prevent jumps of"
						" more than one second\n");
					return -1;
				}
				break;
			case 't':
				OPTS(ppi)->no_adjust = 1;
				break;
			case 'a':
				a = argv[++i];
				cmd_line_parse_two(a, &n1, &n2);
				OPTS(ppi)->ap = n1;
				OPTS(ppi)->ai = n2;
				break;
			case 'w':
				a = argv[++i];
				OPTS(ppi)->s = atoi(a);
				break;
			case 'b':
				a = argv[++i];
				OPTS(ppi)->iface_name = a;
				break;
			case 'l':
				a = argv[++i];
				cmd_line_parse_two(a, &n1, &n2);
				OPTS(ppi)->inbound_latency.nanoseconds = n1;
				OPTS(ppi)->outbound_latency.nanoseconds = n2;
				break;
			case 'o':
				a = argv[++i];
				OPTS(ppi)->cur_utc_ofst = atoi(a);
				break;
			case 'i':
				a = argv[++i];
				OPTS(ppi)->domain_number = atoi(a);
				break;
			case 'y':
				a = argv[++i];
				OPTS(ppi)->sync_intvl = atoi(a);
				break;
			case 'n':
				a = argv[++i];
				OPTS(ppi)->announce_intvl = atoi(a);
				break;
			case 'm':
				a = argv[++i];
				OPTS(ppi)->max_foreign_records = atoi(a);
				if (OPTS(ppi)->max_foreign_records < 1)
					OPTS(ppi)->max_foreign_records = 1;
				break;
			case 'g':
				a = argv[++i];
				OPTS(ppi)->slave_only = 1;
				break;
			case 'v':
				a = argv[++i];
				DSDEF(ppi)->clockQuality.
					offsetScaledLogVariance = atoi(a);
				break;
			case 'r':
				a = argv[++i];
				DSDEF(ppi)->clockQuality.clockAccuracy =
					atoi(a);
				break;
			case 's':
				a = argv[++i];
				DSDEF(ppi)->clockQuality.clockClass =
					atoi(a);
				break;
			case 'p':
				a = argv[++i];
				OPTS(ppi)->prio1 = atoi(a);
				break;
			case 'q':
				a = argv[++i];
				OPTS(ppi)->prio2 = atoi(a);
				break;
			case 'h':
				a = argv[++i];
				OPTS(ppi)->e2e_mode = 1;
				break;
			default:
				cmd_line_print_help();
				return -1;
			}
		}
	 }
	return 0;
}

int pp_open_instance(struct pp_instance *ppi, struct pp_runtime_opts *rt_opts)
{
	if (rt_opts)
		OPTS(ppi) = rt_opts;
	else
		OPTS(ppi) = &default_rt_opts;

	ppi->state = PPS_INITIALIZING;

	return 0;
}

int pp_close_instance(struct pp_instance *ppi)
{
	/* Nothing to do by now */
	return 0;
}
