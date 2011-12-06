/*
 * FIXME: header
 */
#ifndef __PTP_PROTO_H__
#define __PTP_PROTO_H__

#include <stdint.h>
#include <stdarg.h>
#include <arch/arch.h> /* ntohs and so on */
#include <pproto/ieee1588_types.h>
#include <pproto/constants.h>


/*
 * Runtime options. Default values can be overridden by command line.
 */
struct pp_runtime_opts {
	ClockQuality clock_quality;
	TimeInternal inboundLatency, outboundLatency;
	Integer32 max_rst; /* Maximum number of nanoseconds to reset */
	Integer32 max_dly; /* Maximum number of nanoseconds of delay */
	Integer32 ttl;
	char *unicast_addr;
	UInteger32	slave_only:1,
			no_adjust:1,
			display_stats:1,
			csv_stats:1,
			ethernet_mode:1,
			e2e_mode:1,
			ofst_first_updated:1,
			no_rst_clk:1,
			use_syslog:1;
	Integer16 ap, ai;
	Integer16 s;
	Integer16 max_foreign_records;
	Integer16 cur_utc_ofst;
	Integer8 announce_intvl;
	Integer8 sync_intvl;
	UInteger8 prio1;
	UInteger8 prio2;
	UInteger8 domain_number;
	char *iface_name;
	void *arch_opts;

	/*TODO ARCH: arch_opts, for arch-gnu-linux, might include the following:
	 * int log_fd;
	 * char *record_file; [PP_PATH_MAX]
	 * FILE *record_fp;
	 * char *file; [PP_PATH_MAX]
	 */
};
extern struct pp_runtime_opts default_rt_opts; /* preinitialized
                                                * with default values */

/*
 * Communication channel
 */
struct pp_channel {
	union {
		int fd;		/* Posix wants fid descriptor */
		void *custom;	/* Other archs want other stuff */
	};
	void *arch_data;	/* Other arch-private info, if any */
	unsigned char addr[6];	/* Our own MAC address */
	unsigned char peer[6];	/* Our peer's MAC address */
};

/*
 * Structure for the standard protocol
 */
struct pp_instance {
	int state;
	int next_state, next_delay;	/* set by state processing */
	void *arch_data;		/* if arch needs it */
	void *ext_data;			/* if protocol ext needs it */
	struct pp_channel ch;
	struct pp_runtime_opts *rt_opts;

	/* Data sets */
	DSDefault *defaultDS;
	DSCurrent *currentDS;
	DSParent *parentDS;
	DSPort *portDS;
	DSTimeProperties *timePropertiesDS;
};



/* The channel for an instance must be created and possibly destroyed. */
extern int pp_open_instance(struct pp_instance *ppi,
                            struct pp_runtime_opts *rt_opts);

extern int pp_close_instance(struct pp_instance *ppi);

extern int pp_parse_cmdline(struct pp_instance *ppi, int argc, char **argv);

/* Network stuff */
extern int pp_net_init(struct pp_instance *ppi);
extern int pp_net_shutdown(struct pp_instance *ppi);
extern int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len);
extern int pp_send_packet(struct pp_instance *ppi, void *pkt, int len);

/* Timer stuff */
struct pp_timer
{
	int dummy; /* FIXME */
};

extern int pp_timer_init(struct pp_timer *tm);
extern int pp_timer_start(struct pp_timer *tm);
extern int pp_timer_stop(struct pp_timer *tm);
extern int pp_timer_update(struct pp_timer *tm);
extern int pp_timer_expired(struct pp_timer *tm);


/* Get a timestamp */
extern void pp_get_stamp(uint32_t *sptr);

/*
 * The state machine itself is an array of these structures.
 */

struct pp_state_table_item {
	int state;
	int (*f1)(struct pp_instance *ppi, uint8_t *packet, int plen);
	int (*f2)(struct pp_instance *ppi, uint8_t *packet, int plen);
};

extern struct pp_state_table_item pp_state_table[]; /* 0-terminated */



/* Use a typedef, to avoid long prototypes */
typedef int pp_action(struct pp_instance *ppi, uint8_t *packet, int plen);

/* Standard state-machine functions */
extern pp_action pp_initializing, pp_faulty, pp_disabled, pp_listening,
                 pp_pre_master, pp_master, pp_passive, pp_uncalibrated,
                 pp_slave;

/* The engine */
extern int pp_state_machine(struct pp_instance *ppi, uint8_t *packet, int plen);

/*
 * FIXME
 * Whe talk raw sockets on PP_PROTO_NR.
 */
#define PP_PROTO_NR	0xcccc

#endif /* __PTP_PROTO_H__ */
