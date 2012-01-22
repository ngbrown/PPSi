/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

#ifndef __PTP_PROTO_H__
#define __PTP_PROTO_H__

#include <stdint.h>
#include <stdarg.h>
#include <arch/arch.h> /* ntohs and so on */
#include <pptp/lib.h>
#include <pptp/ieee1588_types.h>
#include <pptp/constants.h>

/* Macros for diagnostic prints. Set pp_diag_verbosity as 0 or 1 (PP_V macros
 * disabled/enabled) */
extern int pp_diag_verbosity;
#define PP_FSM(ppi,seq,len) pp_diag_fsm(ppi,seq,len)
#define PP_VFSM(ppi,seq,len) if (pp_diag_verbosity) pp_diag_fsm(ppi,seq,len)
#define PP_VTR(ppi,f,l) if (pp_diag_verbosity) pp_diag_trace(ppi,f,l)
#define PP_ERR(ppi,err) if pp_diag_error(ppi,err)
#define PP_ERR2(ppi,s1,s2) if pp_diag_error_str2(ppi,s1,s2)
#define PP_FATAL(ppi,s1,s2) if pp_diag_fatal(ppi,s1,s2)
#define PP_PRINTF(...) pp_printf(__VA_ARGS__)
#define PP_VPRINTF(...) if (pp_diag_verbosity) pp_printf(__VA_ARGS__)

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
 * Timer. Struct which contains timestamp of timer start and desired interval
 * for timer expiration computation. Both are seconds
 */
struct pp_timer {
	uint32_t start;
	uint32_t interval;
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
	TimeInternal pdelay_ms;
	TimeInternal pdelay_sm;
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
#define PP_NP_LAST	1
struct pp_net_path {
	struct pp_channel ch[2]; /* event and general channel (see above
				  * #define's */
	/* FIXME check if useful Integer32 ucast_addr;*/
	Integer32 mcast_addr;
	Integer32 peer_mcast_addr;
};

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
	DSDefault *defaultDS;
	DSCurrent *currentDS;
	DSParent *parentDS;
	DSPort *portDS;
	DSTimeProperties *timePropertiesDS;
	struct pp_timer *timers[PP_TIMER_ARRAY_SIZE];

	UInteger16 number_foreign_records;
	Integer16  max_foreign_records;
	Integer16  foreign_record_i;
	Integer16  foreign_record_best;
	Boolean  record_update;
	struct pp_frgn_master *frgn_master;
	Octet *buf_out;
	TimeInternal sync_receive_time;
	UInteger16 recv_sync_sequence_id;

	TimeInternal last_rcv_time; /* used to store timestamp retreived from
				     * received packet */
	TimeInternal last_sync_corr_field;
	TimeInternal last_pdelay_req_corr_field;
	TimeInternal last_pdelay_resp_corr_field;
	TimeInternal pdelay_req_send_time;
	TimeInternal pdelay_req_receive_time;
	TimeInternal pdelay_resp_send_time;
	TimeInternal pdelay_resp_receive_time;
	TimeInternal delay_req_send_time;
	TimeInternal delay_req_receive_time;
	Integer8 log_min_delay_req_interval;

	union {
		MsgSync  sync;
		MsgFollowUp  follow;
		MsgDelayReq  req;
		MsgDelayResp resp;
		MsgPDelayReq  preq;
		MsgPDelayResp  presp;
		MsgPDelayRespFollowUp  prespfollow;
		MsgManagement  manage;
		MsgAnnounce  announce;
		MsgSignaling signaling;
	} msg_tmp;
	UInteger16 *sent_seq_id; /* sequence id of the last message sent of the
				  * same type
				  */
	MsgHeader msg_tmp_header;
	MsgHeader pdelay_req_hdr;
	MsgHeader delay_req_hdr;
	UInteger32
		is_from_self:1,
		is_from_cur_par:1,
		waiting_for_follow:1;
};

#define OPTS(x) ((x)->rt_opts)

#define DSDEF(x) ((x)->defaultDS)
#define DSCUR(x) ((x)->currentDS)
#define DSPAR(x) ((x)->parentDS)
#define DSPOR(x) ((x)->portDS)
#define DSPRO(x) ((x)->timePropertiesDS)

#define NP(x) ((x)->net_path)

#define SRV(x) ((x)->servo)

/* The channel for an instance must be created and possibly destroyed. */
extern int pp_open_instance(struct pp_instance *ppi,
			    struct pp_runtime_opts *rt_opts);

extern int pp_close_instance(struct pp_instance *ppi);

extern int pp_parse_cmdline(struct pp_instance *ppi, int argc, char **argv);

/* Network stuff */
extern int pp_net_init(struct pp_instance *ppi);
extern int pp_net_shutdown(struct pp_instance *ppi);
extern int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len,
			  TimeInternal *t);
extern int pp_send_packet(struct pp_instance *ppi, void *pkt, int len,
			  int chtype, int use_pdelay_addr);
			  /* chtype: PP_NP_GEN || PP_NP_EVT */

/* Timers */
extern int pp_timer_init(struct pp_instance *ppi); /* initializes timer common
						      structure */
extern int pp_timer_start(uint32_t interval, struct pp_timer *tm);
extern int pp_timer_stop(struct pp_timer *tm);
extern int pp_timer_expired(struct pp_timer *tm); /* returns 1 when expired */
/* pp_adj_timers is called after pp_set_tstamp and must be defined for those
 * platform who rely on system timestamp for timer expiration handling */
extern void pp_timer_adjust_all(struct pp_instance *ppi, int32_t diff);

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
		     struct pp_frgn_master *frgn_master,
		     struct pp_runtime_opts *rt_opts);

/* msg.c */
extern void msg_pack_header(struct pp_instance *ppi, void *buf);
extern void msg_unpack_header(struct pp_instance *ppi, void *buf);
void *msg_copy_header(MsgHeader *dest, MsgHeader *src);
extern void msg_pack_sync(struct pp_instance *ppi, Timestamp *orig_tstamp);
extern void msg_unpack_sync(void *buf, MsgSync *sync);
extern void msg_pack_announce(struct pp_instance *ppi);
extern void msg_unpack_announce(void *buf, MsgAnnounce *ann);
extern void msg_pack_follow_up(struct pp_instance *ppi,
			       Timestamp *prec_orig_tstamp);
extern void msg_unpack_follow_up(void *buf, MsgFollowUp *flwup);
extern void msg_pack_pdelay_req(struct pp_instance *ppi,
				Timestamp *orig_tstamp);
extern void msg_unpack_pdelay_req(void *buf, MsgPDelayReq *pdelay_req);
extern void msg_pack_delay_req(struct pp_instance *ppi,
			       Timestamp *orig_tstamp);
extern void msg_unpack_delay_req(void *buf, MsgDelayReq *delay_req);
extern void msg_pack_delay_resp(struct pp_instance *ppi,
				MsgHeader *hdr, Timestamp *rcv_tstamp);
extern void msg_unpack_delay_resp(void *buf, MsgDelayResp *resp);
extern void msg_pack_pdelay_resp(struct pp_instance *ppi,
				 MsgHeader *hdr, Timestamp *req_rec_tstamp);
extern void msg_unpack_pdelay_resp(void *buf, MsgPDelayResp *presp);
extern void msg_pack_pdelay_resp_followup(struct pp_instance *ppi,
					  MsgHeader *hdr,
					  Timestamp *resp_orig_tstamp);
extern void msg_unpack_pdelay_resp_followup(void *buf,
	MsgPDelayRespFollowUp *presp_follow);

/* each of them returns 0 if no error and -1 in case of error in send */
extern int msg_issue_announce(struct pp_instance *ppi);
extern int msg_issue_sync(struct pp_instance *ppi);
extern int msg_issue_followup(struct pp_instance *ppi, TimeInternal *time);
extern int msg_issue_delay_req(struct pp_instance *ppi);
extern int msg_issue_pdelay_req(struct pp_instance *ppi);
extern int msg_issue_pdelay_resp(struct pp_instance *ppi, TimeInternal *time,
			MsgHeader *hdr);
extern int msg_issue_delay_resp(struct pp_instance *ppi, TimeInternal *time);
extern int msg_issue_pdelay_resp_follow_up(struct pp_instance *ppi,
			TimeInternal *time);


/* Functions for timestamp handling (internal to protocol format conversion*/
/* FIXME: add prefix in function name? */
extern void int64_to_TimeInternal(Integer64 bigint, TimeInternal *internal);
extern int from_TimeInternal(TimeInternal *internal, Timestamp *external);
extern int to_TimeInternal(TimeInternal *internal, Timestamp *external);
extern void add_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y);
extern void sub_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y);
extern void set_TimeInternal(TimeInternal *t, Integer32 s, Integer32 ns);
extern void display_TimeInternal(const char *label, TimeInternal *t);

/* Get and Set system timestamp */
extern void pp_get_tstamp(TimeInternal *t);
extern int32_t pp_set_tstamp(TimeInternal *t);


/* Virtualization of Linux adjtimex (or BSD adjtime) system clock time
 * adjustment. Boolean: returns 1 in case of success and 0 if failure */
extern int pp_adj_freq(Integer32 adj);
extern const Integer32 PP_ADJ_FREQ_MAX;

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
 * FIXME Whe talk raw sockets on PP_PROTO_NR.
 */
#define PP_PROTO_NR	0xcccc

#endif /* __PTP_PROTO_H__ */
