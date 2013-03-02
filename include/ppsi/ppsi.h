/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

#ifndef __PPSI_PPSI_H__
#define __PPSI_PPSI_H__

#include <stdint.h>
#include <stdarg.h>
#include <arch/arch.h> /* ntohs and so on */
#include <ppsi/diag-macros.h>
#include <ppsi/lib.h>
#include <ppsi/ieee1588_types.h>
#include <ppsi/constants.h>
#include <ppsi/jiffies.h>

/*
 * Runtime options. Default values can be overridden by command line.
 */
struct pp_runtime_opts {
	ClockQuality clock_quality;
	TimeInternal inbound_latency, outbound_latency;
	Integer32 max_rst; /* Maximum number of nanoseconds to reset */
	Integer32 max_dly; /* Maximum number of nanoseconds of delay */
	Integer32 ttl;
	char *unicast_addr;
	UInteger32	slave_only:1,
			master_only:1,
			no_adjust:1,
			display_stats:1,
			csv_stats:1,
			ethernet_mode:1,
			/* e2e_mode:1, -- no more: we only support e2e */
			/* gptp_mode:1, -- no more: peer-to-peer unsupported */
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
};
extern struct pp_runtime_opts default_rt_opts; /* preinited with defaults */

/*
 * Communication channel. Is the abstraction of a posix socket, so that
 * this struct is platform independent
 */
struct pp_channel {
	union {
		int fd;		/* Posix wants fid descriptor */
		void *custom;	/* Other archs want other stuff */
	};
	void *arch_data;	/* Other arch-private info, if any */
	unsigned char addr[6];	/* Our own MAC address */
	unsigned char peer[6];	/* Our peer's MAC address */
	int pkt_present;
};

/*
 * Foreign master record. Used to manage Foreign masters
 */
struct pp_frgn_master {
	PortIdentity port_identity;
	UInteger16 ann_messages;

	//This one is not in the spec
	MsgAnnounce ann;
	MsgHeader hdr;
};

/*
 * Servo. Structs which contain filters for delay average computation. They
 * are used in servo.c src, where specific function for time setting of the
 * machine are implemented.
 *
 * pp_ofm_fltr: The FIR filtering of the offset from master input is a simple,
 * two-sample average
 *
 * pp_owd_fltr: It is a variable cutoff/delay low-pass, infinite impulse
 * response (IIR) filter. The one-way delay filter has the difference equation:
 * s*y[n] - (s-1)*y[n-1] = x[n]/2 + x[n-1]/2,
 * where increasing the stiffness (s) lowers the cutoff and increases the delay.
 */
struct pp_ofm_fltr {
	Integer32 nsec_prev;
	Integer32 y;
};

struct pp_owd_fltr {
	Integer32 nsec_prev;
	Integer32 y;
	Integer32 s_exp;
};

struct pp_servo {
	/* TODO check. Which is the difference between m_to_s_dly (which
	 * comes from ptpd's master_to_slave_delay) and delay_ms (which comes
	 * from ptpd's delay_MS? Seems like ptpd actually uses only delay_MS.
	 * The same of course must be checked for their equivalents,
	 * s_to_m_dly and delay_sm
	 */
	TimeInternal m_to_s_dly;
	TimeInternal s_to_m_dly;
	TimeInternal delay_ms;
	TimeInternal delay_sm;
	Integer32 obs_drift;
	struct pp_owd_fltr owd_fltr;
	struct pp_ofm_fltr ofm_fltr;
};

/*
 * Net Path. Struct which contains the network configuration parameters and
 * the event/general channels (sockets on most platforms, see above)
 */

#define PP_NP_GEN	0
#define PP_NP_EVT	1
#define PP_NP_LAST	2
struct pp_net_path {
	struct pp_channel ch[2]; /* event and general channel (see above
				  * #define's */
	/* FIXME check if useful Integer32 ucast_addr;*/
	Integer32 mcast_addr;
	Integer32 peer_mcast_addr;
	int proto_ofst;
	int inited;
};
#define PROTO_HDR(x) ( (x) - NP(ppi)->proto_ofst);
#define PROTO_PAYLOAD(x) ( (x) + NP(ppi)->proto_ofst);

/*
 * Structure for the standard protocol
 */
struct pp_instance {
	int state;
	int next_state, next_delay, is_new_state; /* set by state processing */
	void *arch_data;		/* if arch needs it */
	void *ext_data;			/* if protocol ext needs it */
	struct pp_runtime_opts *rt_opts;
	struct pp_net_path *net_path;
	struct pp_servo *servo;

	/* Data sets */
	DSDefault *defaultDS;			/* page 65 */
	DSCurrent *currentDS;			/* page 67 */
	DSParent *parentDS;			/* page 68 */
	DSPort *portDS;				/* page 72 */
	DSTimeProperties *timePropertiesDS;	/* page 70 */
	unsigned long timeouts[__PP_TO_ARRAY_SIZE];
	UInteger16 number_foreign_records;
	Integer16  foreign_record_i;
	Integer16  foreign_record_best;
	Boolean  record_update;
	struct pp_frgn_master *frgn_master;
	Octet *buf_out;
	TimeInternal sync_receive_time;
	UInteger16 recv_sync_sequence_id;

	TimeInternal last_rcv_time; /* used to store timestamp retreived from
				     * received packet */
	TimeInternal last_snt_time; /* used to store timestamp retreived from
				     * sent packet */
	TimeInternal last_sync_corr_field;
	TimeInternal delay_req_send_time;
	TimeInternal delay_req_receive_time;
	Integer8 log_min_delay_req_interval;

	union {
		MsgSync  sync;
		MsgFollowUp  follow;
		MsgDelayResp resp;
		MsgAnnounce  announce;
	} msg_tmp;
	UInteger16 *sent_seq_id; /* sequence id of the last message sent of the
				  * same type
				  */
	MsgHeader msg_tmp_header;
	MsgHeader delay_req_hdr;
	UInteger32
		is_from_cur_par:1,
		waiting_for_follow:1;
};

/* We use data sets a lot, so have these helpers */
static inline struct pp_runtime_opts *OPTS(struct pp_instance *ppi)
{
	return ppi->rt_opts;
}

static inline struct DSDefault *DSDEF(struct pp_instance *ppi)
{
	return ppi->defaultDS;
}

static inline struct DSCurrent *DSCUR(struct pp_instance *ppi)
{
	return ppi->currentDS;
}

static inline struct DSParent *DSPAR(struct pp_instance *ppi)
{
	return ppi->parentDS;
}

static inline struct DSPort *DSPOR(struct pp_instance *ppi)
{
	return ppi->portDS;
}

static inline struct DSTimeProperties *DSPRO(struct pp_instance *ppi)
{
	return ppi->timePropertiesDS;
}

static inline struct pp_net_path *NP(struct pp_instance *ppi)
{
	return ppi->net_path;
}

static inline struct pp_servo *SRV(struct pp_instance *ppi)
{
	return ppi->servo;
}


/*
 * Each extension should fill this structure that is used to augment
 * the standard states and avoid code duplications. Please remember
 * that proto-standard functions are picked as a fall-back when non
 * extension-specific code is provided. The set of hooks here is designed
 * based on what White Rabbit does. If you add more please remember to
 * allow NULL pointers.
 */
struct pp_ext_hooks {
	int (*init)(struct pp_instance *ppi, unsigned char *pkt, int plen);
	int (*open)(struct pp_instance *ppi, struct pp_runtime_opts *rt_opts);
	int (*close)(struct pp_instance *ppi);
	int (*listening)(struct pp_instance *ppi, unsigned char *pkt, int plen);
	int (*master_msg)(struct pp_instance *ppi, unsigned char *pkt,
			  int plen, int msgtype);
	int (*new_slave)(struct pp_instance *ppi, unsigned char *pkt, int plen);
	int (*update_delay)(struct pp_instance *ppi);
	void (*s1)(struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann);
	int (*execute_slave)(struct pp_instance *ppi);
	void (*handle_announce)(struct pp_instance *ppi);
	int (*handle_followup)(struct pp_instance *ppi, TimeInternal *orig,
			       TimeInternal *correction_field);
	int (*pack_announce)(struct pp_instance *ppi);
	void (*unpack_announce)(void *buf, MsgAnnounce *ann);
};

extern struct pp_ext_hooks pp_hooks; /* The one for the extension we build */


/*
 * Network methods are encapsulated in a structure, so each arch only needs
 * to provide that structure. This should simplify management overall.
 */
struct pp_network_operations {
	int (*init)(struct pp_instance *ppi);
	int (*exit)(struct pp_instance *ppi);
	int (*recv)(struct pp_instance *ppi, void *pkt, int len,
		    TimeInternal *t);
	/* chtype here is PP_NP_GEN or PP_NP_EVT -- use_pdelay must be 0 */
	int (*send)(struct pp_instance *ppi, void *pkt, int len,
		    TimeInternal *t, int chtype, int use_pdelay_addr);
};

extern struct pp_network_operations pp_net_ops;


/*
 * Time operations, like network operations above, are encapsulated.
 * They may live in their own time-<name> subdirectory.
 *
 * Maybe this structure will need updating, to pass ppi as well
 */
struct pp_time_operations {
	int (*get)(TimeInternal *t); /* returns error code */
	int (*set)(TimeInternal *t); /* returns error code */
	/* freq_ppm is "scaled-ppm" like the argument of adjtimex(2) */
	int (*adjust)(long offset_ns, long freq_ppm);
};

/* IF the configuration prevents jumps, this is the max jump (0.5ms) */
#define  PP_ADJ_NS_MAX		(500*1000)

/* In geeneral, we can't adjust the rate by more than 200ppm */
#define  PP_ADJ_FREQ_MAX	(200 << 16)

extern struct pp_time_operations pp_t_ops;


/*
 * Timeouts. I renamed from "timer" to "timeout" to avoid
 * misread/miswrite with the time operations above. A timeout, actually,
 * is just a number that must be compared with the current counter.
 * So we don't need struct operations, as it is one function only.
 */
extern unsigned long pp_calc_timeout(int millisec); /* cannot return zero */

static inline void pp_timeout_set(struct pp_instance *ppi, int index,
				  int millisec)
{
	ppi->timeouts[index] = pp_calc_timeout(millisec);
}

extern void pp_timeout_rand(struct pp_instance *ppi, int index, int logval);

static inline void pp_timeout_clr(struct pp_instance *ppi, int index)
{
	ppi->timeouts[index] = 0;
}

extern void pp_timeout_log(struct pp_instance *ppi, int index);

static inline int pp_timeout(struct pp_instance *ppi, int index)
{
	int ret = ppi->timeouts[index] &&
		time_after_eq(pp_calc_timeout(0), ppi->timeouts[index]);

	if (ret && pp_verbose_time)
		pp_timeout_log(ppi, index);
	return ret;
}

static inline int pp_timeout_z(struct pp_instance *ppi, int index)
{
	int ret = pp_timeout(ppi, index);

	if (ret)
		pp_timeout_clr(ppi, index);
	return ret;
}

/* This functions are generic, not architecture-specific */
extern void pp_timeout_set(struct pp_instance *ppi, int index, int millisec);
extern void pp_timeout_clr(struct pp_instance *ppi, int index);
extern int pp_timeout(struct pp_instance *ppi, int index); /* 1 == timeout */


/* The channel for an instance must be created and possibly destroyed. */
extern int pp_open_instance(struct pp_instance *ppi,
			    struct pp_runtime_opts *rt_opts);

extern int pp_close_instance(struct pp_instance *ppi);

extern int pp_parse_cmdline(struct pp_instance *ppi, int argc, char **argv);

/* Servo */
extern void pp_init_clock(struct pp_instance *ppi);
extern void pp_update_delay(struct pp_instance *ppi,
			    TimeInternal *correction_field);
extern void pp_update_peer_delay(struct pp_instance *ppi,
			  TimeInternal *correction_field, int two_step);
extern void pp_update_offset(struct pp_instance *ppi,
			     TimeInternal *send_time,
			     TimeInternal *recv_time,
			     TimeInternal *correctionField);
extern void pp_update_clock(struct pp_instance *ppi);


/* bmc.c */
extern void m1(struct pp_instance *ppi);
extern void s1(struct pp_instance *ppi, MsgHeader *header, MsgAnnounce *ann);
extern UInteger8 bmc(struct pp_instance *ppi,
		     struct pp_frgn_master *frgn_master);

/* msg.c */
extern void msg_pack_header(struct pp_instance *ppi, void *buf);
extern int __attribute__((warn_unused_result))
	msg_unpack_header(struct pp_instance *ppi, void *buf);
void *msg_copy_header(MsgHeader *dest, MsgHeader *src);
extern void msg_pack_sync(struct pp_instance *ppi, Timestamp *orig_tstamp);
extern void msg_unpack_sync(void *buf, MsgSync *sync);
extern int msg_pack_announce(struct pp_instance *ppi);
extern void msg_unpack_announce(void *buf, MsgAnnounce *ann);
extern void msg_pack_follow_up(struct pp_instance *ppi,
			       Timestamp *prec_orig_tstamp);
extern void msg_unpack_follow_up(void *buf, MsgFollowUp *flwup);
extern void msg_pack_delay_req(struct pp_instance *ppi,
			       Timestamp *orig_tstamp);
extern void msg_unpack_delay_req(void *buf, MsgDelayReq *delay_req);
extern void msg_pack_delay_resp(struct pp_instance *ppi,
				MsgHeader *hdr, Timestamp *rcv_tstamp);
extern void msg_unpack_delay_resp(void *buf, MsgDelayResp *resp);

/* each of them returns 0 if no error and -1 in case of error in send */
extern int msg_issue_announce(struct pp_instance *ppi);
extern int msg_issue_sync(struct pp_instance *ppi);
extern int msg_issue_followup(struct pp_instance *ppi, TimeInternal *time);
extern int msg_issue_delay_req(struct pp_instance *ppi);
extern int msg_issue_delay_resp(struct pp_instance *ppi, TimeInternal *time);


/* Functions for timestamp handling (internal to protocol format conversion*/
/* FIXME: add prefix in function name? */
extern void int64_to_TimeInternal(Integer64 bigint, TimeInternal *internal);
extern int from_TimeInternal(TimeInternal *internal, Timestamp *external);
extern int to_TimeInternal(TimeInternal *internal, Timestamp *external);
extern void add_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y);
extern void sub_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y);
extern void display_TimeInternal(const char *label, TimeInternal *t);
extern void div2_TimeInternal(TimeInternal *r);

/*
 * The state machine itself is an array of these structures.
 */

struct pp_state_table_item {
	int state;
	char *name;
	int (*f1)(struct pp_instance *ppi, uint8_t *packet, int plen);
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

#endif /* __PPSI_PPSI_H__ */
