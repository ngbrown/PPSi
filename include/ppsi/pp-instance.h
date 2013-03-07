#ifndef __PPSI_PP_INSTANCE_H__
#define __PPSI_PP_INSTANCE_H__

/*
 * The "instance structure is the most important object in ppsi: is is
 * now part of a separate header because ppsi.h must refer to it, and
 * to diagnostic macros, but also diag-macros.h needs to look at
 * ppi->flags.
 */
#ifndef __PPSI_PPSI_H__
#warning "Please include <ppsi/ppsi.h>, don't refer to pp-instance.h"
#endif

/*
 * Runtime options. Default values can be overridden by command line.
 */
struct pp_runtime_opts {
	ClockQuality clock_quality;
	TimeInternal inbound_latency, outbound_latency;
	Integer32 max_rst; /* Maximum number of nanoseconds to reset */
	Integer32 max_dly; /* Maximum number of nanoseconds of delay */
	Integer32 ttl;
	UInteger32	slave_only:1,
			master_only:1,
			no_adjust:1,
			ethernet_mode:1,
			/* e2e_mode:1, -- no more: we only support e2e */
			/* gptp_mode:1, -- no more: peer-to-peer unsupported */
			ofst_first_updated:1,
			no_rst_clk:1;
	Integer16 ap, ai;
	Integer16 s;
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

enum {
	PP_NP_GEN =	0,
	PP_NP_EVT,
	__NR_PP_NP,
};

struct pp_net_path {
	struct pp_channel ch[__NR_PP_NP];	/* general and event ch */
	Integer32 mcast_addr;			/* FIXME: only ipv4/udp */

	int ptp_offset;
};

/*
 * Structure for the individual ppsi link
 */
struct pp_instance {
	int state;
	int next_state, next_delay, is_new_state; /* set by state processing */
	void *arch_data;		/* if arch needs it */
	void *ext_data;			/* if protocol ext needs it */
	struct pp_runtime_opts *rt_opts;
	struct pp_servo *servo;
	unsigned long flags;		/* ppi-specific flags (diag mainly) */

	/* Operations that may be different in each instance */
	struct pp_network_operations *n_ops;
	struct pp_time_operations *t_ops;

	/*
	 * We host the buffer for this fsm here, tracking both frame
	 * and payload. send/recv get the frame, pack/unpack the payload
	 */
	unsigned char tx_buffer[PP_MAX_FRAME_LENGTH];
	unsigned char rx_buffer[PP_MAX_FRAME_LENGTH];
	void *tx_frame, *rx_frame, *tx_ptp, *rx_ptp;

	/* The net_path used to be allocated separately, but there's no need */
	struct pp_net_path np;

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
	struct pp_frgn_master frgn_master[PP_NR_FOREIGN_RECORDS];
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
	UInteger16 sent_seq[__PP_NR_MESSAGES_TYPES]; /* last sent this type */
	MsgHeader received_ptp_header;
	MsgHeader delay_req_hdr;
	UInteger32
		is_from_cur_par:1,
		waiting_for_follow:1;
};

#endif /* __PPSI_PP_INSTANCE_H__ */