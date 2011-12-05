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
 * This is the struct handling runtime options. Default values can be
 * overridden by command line
 */
struct pp_runtime_opts {
	/* TODO */
};

struct pp_net_path {
	union {
		int evt_sock;
		void *custom;
	};
	int32_t gen_sock;
	int32_t mcast_addr;
	int32_t peer_mcast_addr;
	int32_t ucast_addr;
};

extern struct pp_runtime_opts default_rt_opts; /* preinitialized
                                                * with default values */


/*
 * This is the communication channel. Either a socket (Posix stuff)
 * or another implementation-specific thin, possibly allocated.
 * For consistency, this is a struct not a union. gcc allows unnamed
 * unions as structure fields, and we are gcc-specific anyways.
 */
struct pp_channel {
	union {
		int fd;		/* Poisx wants fide descriptor */
		void *custom;	/* Other archs want other stuff */
	};
	void *arch_data;	/* Other arch-private info, if any */
	unsigned char addr[6];	/* Our own MAC address */
	unsigned char peer[6];	/* Our peer's MAC address */
};

/*
 * This is the structure for the standard protocol.
 * Extensions may allocate a bigger structure that includes this one.
 * The encompassing structure will be at the same address, or container_of()
 * may be implemented and used
 */
struct pp_instance {
	int state;
	int next_state, next_delay;	/* set by state processing */
	void *arch_data;		/* if arch needs it */
	void *ext_data;			/* if protocol ext needs it */
	struct pp_channel ch;
	struct pp_runtime_opts *rt_opts;
	struct pp_clock *ppc;

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

/* network stuff */
extern int pp_net_init(struct pp_instance *ppi);
extern int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len);
extern int pp_send_packet(struct pp_instance *ppi, void *pkt, int len);


/* Get a timestamp */
extern void pp_get_stamp(uint32_t *sptr);


/*
 * The state machine itself is an array of these structures.
 * Each state in the machine has an array item. Both functions are
 * called (one is expected to be the standard one, the other the extension).
 *
 * The return value is 0 or a negative error code, but functions should
 * also set the fields next_state and next_delay. Processing for the next
 * state is entered when a packet arrives or when the delay expires.
 */

struct pp_state_table_item {
	int state;
	int (*f1)(struct pp_instance *ppi, uint8_t *packet, int plen);
	int (*f2)(struct pp_instance *ppi, uint8_t *packet, int plen);
};

extern struct pp_state_table_item pp_state_table[]; /* 0-terminated */

enum pp_std_states {
	PPS_END_OF_TABLE = 0,
	PPS_INITIALIZING,
	PPS_FAULTY,
	PPS_DISABLED,
	PPS_LISTENING,
	PPS_PRE_MASTER,
	PPS_MASTER,
	PPS_PASSIVE,
	PPS_UNCALIBRATED,
	PPS_SLAVE,
};

/* Use a typedef, to avoid long prototypes -- I'm not sure I like it */
typedef int pp_action(struct pp_instance *ppi, uint8_t *packet, int plen);

/* Standard state-machine functions */
extern pp_action pp_initializing, pp_faulty, pp_disabled, pp_listening,
                 pp_pre_master, pp_master, pp_passive, pp_uncalibrated,
                 pp_slave;

/* The engine */
extern int pp_state_machine(struct pp_instance *ppi, uint8_t *packet, int plen);

/*
 * FIXME
 * What follows is the protocol itself. Definition of packet and such stuff.
 * Whe talk raw sockets on PP_PROTO_NR.
 */
#define PP_PROTO_NR	0xcccc

#endif /* __PTP_PROTO_H__ */
