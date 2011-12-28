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
struct pp_channel
{
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
struct pp_frgn_master
{
	PortIdentity port_identity;
	UInteger16 ann_messages;

	//This one is not in the spec
	MsgAnnounce ann;
	MsgHeader hdr;
};

/*
 * Timer
 */
struct pp_timer
{
	uint32_t start;
	uint32_t interval;
};

/*
 * Net Path
 */
struct pp_net_path
{
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
	struct timeval tv;

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
	int is_from_self;

};

#define DSDEF(x) x->defaultDS
#define DSCUR(x) x->currentDS
#define DSPAR(x) x->parentDS
#define DSPOR(x) x->portDS
#define DSPRO(x) x->timePropertiesDS


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
extern UInteger32 pp_htonl(UInteger32 hostlong);
extern UInteger16 pp_htons(UInteger16 hostshort);


/* Timers */
extern int pp_timer_init(struct pp_instance *ppi); /* initializes timer common
                                                      structure */
extern int pp_timer_start(uint32_t interval, struct pp_timer *tm);
extern int pp_timer_stop(struct pp_timer *tm);
extern int pp_timer_expired(struct pp_timer *tm); /* returns 1 when expired */


/* bmc.c */
extern void m1(struct pp_instance *ppi);
extern void s1(MsgHeader *header, MsgAnnounce *ann, struct pp_instance *ppi);
extern UInteger8 bmc(struct pp_frgn_master *frgn_master,
	      struct pp_runtime_opts *rt_opts, struct pp_instance *ppi);

/* msg.c */
extern void msg_pack_header(void *buf, struct pp_instance *ppi);
extern void msg_unpack_header(void *buf, struct pp_instance *ppi);
extern void msg_pack_sync(void *buf, Timestamp *orig_tstamp,
		struct pp_instance *ppi);
extern void msg_unpack_sync(void *buf, MsgSync *sync);
extern void msg_pack_announce(void *buf, struct pp_instance *ppi);
extern void msg_unpack_announce(void *buf, MsgAnnounce *ann);
extern void msg_pack_follow_up(void *buf, Timestamp *prec_orig_tstamp,
		struct pp_instance *ppi);
extern void msg_unpack_follow_up(void *buf, MsgFollowUp *flwup);
extern void msg_pack_pdelay_req(void *buf, Timestamp *orig_tstamp,
		struct pp_instance *ppi);
extern void msg_unpack_pdelay_req(void *buf, MsgPDelayReq *pdelay_req);
extern void msg_pack_delay_req(void *buf, Timestamp *orig_tstamp,
		struct pp_instance *ppi);
extern void msg_unpack_delay_req(void *buf, MsgDelayReq *delay_req);
extern void msg_pack_delay_resp(void *buf, MsgHeader *hdr,
		Timestamp *rcv_tstamp, struct pp_instance *ppi);
extern void msg_unpack_delay_resp(void *buf, MsgDelayResp *resp);
extern void msg_pack_pdelay_resp(void *buf, MsgHeader *hdr,
		Timestamp *req_rec_tstamp, struct pp_instance *ppi);
extern void msg_unpack_pdelay_resp(void *buf, MsgPDelayResp *presp);
extern void msg_pack_pdelay_resp_followup(void *buf, MsgHeader *hdr,
	Timestamp *resp_orig_tstamp, struct pp_instance* ppi);
extern void msg_unpack_pdelay_resp_followup(void *buf,
	MsgPDelayRespFollowUp *presp_follow);

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
