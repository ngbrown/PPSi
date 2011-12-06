/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#ifndef __PTP_PROTO_H__
#define __PTP_PROTO_H__

#include <stdint.h>
#include <stdarg.h>
#include <arch/arch.h> /* ntohs and so on */
#include <pproto/ieee1588_types.h>


/*
 * Runtime options. Default values can be overridden by command line.
 */
struct pp_runtime_opts {
	/* TODO */
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
extern int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len);
extern int pp_send_packet(struct pp_instance *ppi, void *pkt, int len);


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
