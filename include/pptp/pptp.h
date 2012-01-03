/*
 * FIXME: header
 */
#ifndef __PTP_PROTO_H__
#define __PTP_PROTO_H__

#include <stdint.h>
#include <stdarg.h>
#include <arch/arch.h> /* ntohs and so on */
#include <pptp/lib.h>
#include <pptp/ieee1588_types.h>
#include <pptp/constants.h>


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

	/*TODO ARCH: arch_opts, for arch-gnu-linux, might include the following:
	 * int log_fd;
	 * char *record_file; [PP_PATH_MAX]
	 * FILE *record_fp;
	 * char *file; [PP_PATH_MAX]
	 */
};
extern struct pp_runtime_opts default_rt_opts; /* preinited with defaults */

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
 * Timer
 */
struct pp_timer {
	uint32_t start;
	uint32_t interval;
};

/*
 * Net Path
 */
struct pp_net_path {
	struct pp_channel evt_ch;
	struct pp_channel gen_ch;
	Integer32 ucast_addr;
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

	/* Data sets */
	DSDefault *defaultDS;
	DSCurrent *currentDS;
	DSParent *parentDS;
	DSPort *portDS;
	DSTimeProperties *timePropertiesDS;
	struct pp_timer *timers[PP_TIMER_ARRAY_SIZE];

	/* FIXME: check the variables from now on. Now inherited from ptpd src*/
	UInteger16 number_foreign_records;
	Integer16  max_foreign_records;
	Integer16  foreign_record_i;
	Integer16  foreign_record_best;
	Boolean  record_update;
	struct pp_frgn_master *frgn_master;
	Octet *buf_out;
	Octet *buf_in;	/* FIXME really useful? Probably not*/
	TimeInternal sync_receive_time;
	UInteger16 recv_sync_sequence_id;
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

#define DSDEF(x) ((x)->defaultDS)
#define DSCUR(x) ((x)->currentDS)
#define DSPAR(x) ((x)->parentDS)
#define DSPOR(x) ((x)->portDS)
#define DSPRO(x) ((x)->timePropertiesDS)


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

/* Timers */
extern int pp_timer_init(struct pp_instance *ppi); /* initializes timer common
						      structure */
extern int pp_timer_start(uint32_t interval, struct pp_timer *tm);
extern int pp_timer_stop(struct pp_timer *tm);
extern int pp_timer_expired(struct pp_timer *tm); /* returns 1 when expired */

/* Servo */
extern void pp_init_clock(struct pp_instance *ppi);
extern void pp_update_offset(struct pp_instance *ppi,
			     TimeInternal *send_time,
			     TimeInternal *recv_time,
			     TimeInternal *correctionField);
			/* FIXME: offset_from_master_filter: put it in ppi */
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
extern void msg_pack_sync(struct pp_instance *ppi, void *buf,
			  Timestamp *orig_tstamp);
extern void msg_unpack_sync(void *buf, MsgSync *sync);
extern void msg_pack_announce(struct pp_instance *ppi, void *buf);
extern void msg_unpack_announce(void *buf, MsgAnnounce *ann);
extern void msg_pack_follow_up(struct pp_instance *ppi, void *buf,
			       Timestamp *prec_orig_tstamp);
extern void msg_unpack_follow_up(void *buf, MsgFollowUp *flwup);
extern void msg_pack_pdelay_req(struct pp_instance *ppi, void *buf,
				Timestamp *orig_tstamp);
extern void msg_unpack_pdelay_req(void *buf, MsgPDelayReq *pdelay_req);
extern void msg_pack_delay_req(struct pp_instance *ppi, void *buf,
			       Timestamp *orig_tstamp);
extern void msg_unpack_delay_req(void *buf, MsgDelayReq *delay_req);
extern void msg_pack_delay_resp(struct pp_instance *ppi, void *buf,
				MsgHeader *hdr, Timestamp *rcv_tstamp);
extern void msg_unpack_delay_resp(void *buf, MsgDelayResp *resp);
extern void msg_pack_pdelay_resp(struct pp_instance *ppi, void *buf,
				 MsgHeader *hdr, Timestamp *req_rec_tstamp);
extern void msg_unpack_pdelay_resp(void *buf, MsgPDelayResp *presp);
extern void msg_pack_pdelay_resp_followup(struct pp_instance *ppi, void *buf,
					  MsgHeader *hdr,
					  Timestamp *resp_orig_tstamp);
extern void msg_unpack_pdelay_resp_followup(void *buf,
	MsgPDelayRespFollowUp *presp_follow);


/* arith.c */
/* FIXME: add prefix in function name? */
extern void int64_to_TimeInternal(Integer64 bigint, TimeInternal *internal);
extern void from_TimeInternal(TimeInternal *internal, Timestamp *external);
extern void to_TimeInternal(TimeInternal *internal, Timestamp *external);
extern void normalize_TimeInternal(TimeInternal *r);
extern void add_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y);
extern void sub_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y);


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
