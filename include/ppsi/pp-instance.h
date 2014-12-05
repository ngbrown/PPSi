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
	int flags;		/* see below */
	Integer16 ap, ai;
	Integer16 s;
	Integer8 announce_intvl;
	Integer8 sync_intvl;
	UInteger8 prio1;
	UInteger8 prio2;
	UInteger8 domain_number;
	void *arch_opts;
};

/*
 * Flags for the above structure
 */
#define PP_FLAG_NO_ADJUST  0x01
#define PP_FLAG_NO_RESET   0x02
/* I'd love to use inlines, but we still miss some structure at this point*/
#define pp_can_adjust(ppi)      (!(OPTS(ppi)->flags & PP_FLAG_NO_ADJUST))
#define pp_can_reset_clock(ppi) (!(OPTS(ppi)->flags & PP_FLAG_NO_RESET))

/* slave_only:1, -- moved to ppi, no more global */
/* master_only:1, -- moved to ppi, no more global */
/* ethernet_mode:1, -- moved to ppi, no more global */
/* e2e_mode:1, -- no more: we only support e2e */
/* gptp_mode:1, -- no more: peer-to-peer unsupported */



/* We need a globally-accessible structure with preset defaults */
extern struct pp_runtime_opts __pp_default_rt_opts;

/*
 * Communication channel. Is the abstraction of a unix socket, so that
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
 * Foreign master record. Used to manage Foreign masters. In the specific
 * it is called foreignMasterDS, see 9.3.2.4
 */
struct pp_frgn_master {
	PortIdentity port_id;	/* used to identify old/new masters */

	/* We don't need all fields of the following ones */
	MsgAnnounce ann;
	MsgHeader hdr;
};

/*
 * Servo. Structs which contain filters for delay average computation. They
 * are used in servo.c src, where specific function for time setting of the
 * machine are implemented.
 *
 * pp_avg_fltr: It is a variable cutoff/delay low-pass, infinite impulse
 * response (IIR) filter. The meanPathDelay filter has the difference equation:
 * s*y[n] - (s-1)*y[n-1] = x[n]/2 + x[n-1]/2,
 * where increasing the stiffness (s) lowers the cutoff and increases the delay.
 */
struct pp_avg_fltr {
	Integer32 m; /* magnitude */
	Integer32 y;
	Integer32 s_exp;
};

struct pp_servo {
	TimeInternal m_to_s_dly;
	TimeInternal s_to_m_dly;
	long long obs_drift;
	struct pp_avg_fltr mpd_fltr;
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
 * Struct containg the result of ppsi.conf parsing: one for each link
 * (see lib/conf.c). Actually, protocol and role are in the main ppi.
 */
struct pp_instance_cfg {
	char port_name[16];
	char iface_name[16];
	int ext;   /* 0: none, 1: whiterabbit */ /* FIXME extension enumeration */
};

/*
 * Structure for the individual ppsi link
 */
struct pp_instance {
	int state;
	int next_state, next_delay, is_new_state; /* set by state processing */
	void *arch_data;		/* if arch needs it */
	void *ext_data;			/* if protocol ext needs it */
	unsigned long d_flags;		/* diagnostics, ppi-specific flags */
	unsigned char flags,		/* protocol flags (see below) */
		role,			/* same as in config file */
		proto;			/* same as in config file */

	/* Pointer to global instance owning this pp_instance*/
	struct pp_globals *glbs;

	/* Operations that may be different in each instance */
	struct pp_network_operations *n_ops;
	struct pp_time_operations *t_ops;

	/*
	 * The buffer for this fsm are allocated. Then we need two
	 * extra pointer to track separately the frame and payload.
	 * So send/recv use the frame, pack/unpack use the payload.
	 */
	void *__tx_buffer, *__rx_buffer;
	void *tx_frame, *rx_frame, *tx_ptp, *rx_ptp;

	/* The net_path used to be allocated separately, but there's no need */
	struct pp_net_path np;

	/* Times, for the various offset computations */
	TimeInternal t1, t2, t3, t4;			/* *the* stamps */
	TimeInternal cField;				/* transp. clocks */
	TimeInternal last_rcv_time, last_snt_time;	/* two temporaries */

	/* Page 85: each port shall maintain an implementation-specific
	 * foreignMasterDS data set for the purposes of qualifying Announce
	 * messages */
	UInteger16 frgn_rec_num;
	Integer16  frgn_rec_best;
	struct pp_frgn_master frgn_master[PP_NR_FOREIGN_RECORDS];

	DSPort *portDS;				/* page 72 */

	unsigned long timeouts[__PP_TO_ARRAY_SIZE];
	UInteger16 recv_sync_sequence_id;

	Integer8 log_min_delay_req_interval;

	UInteger16 sent_seq[__PP_NR_MESSAGES_TYPES]; /* last sent this type */
	MsgHeader received_ptp_header;
	char *iface_name; /* for direct actions on hardware */
	char *port_name; /* for diagnostics, mainly */
	int port_idx;

	struct pp_instance_cfg cfg;
};
/* The following things used to be bit fields. Other flags are now enums */
#define PPI_FLAG_FROM_CURRENT_PARENT	0x01
#define PPI_FLAG_WAITING_FOR_F_UP	0x02


struct pp_globals_cfg {
	int cfg_items;			/* Remember how many we parsed */
	int cur_ppi_n;	/* Remember which instance we are configuring */
};

/*
 * Structure for the multi-port ppsi instance.
 */
struct pp_globals {
	struct pp_instance *pp_instances;

	struct pp_servo *servo;

	/* Real time options */
	struct pp_runtime_opts *rt_opts;

	/* Data sets */
	DSDefault *defaultDS;			/* page 65 */
	DSCurrent *currentDS;			/* page 67 */
	DSParent *parentDS;			/* page 68 */
	DSTimeProperties *timePropertiesDS;	/* page 70 */

	/* Index of the pp_instance receiving the "Ebest" clock */
	int ebest_idx;
	int ebest_updated; /* set to 1 when ebest_idx changes */

	int nlinks;
	int max_links;
	struct pp_globals_cfg cfg;

	int rxdrop, txdrop;		/* fault injection, per thousand */

	void *arch_data;		/* if arch needs it */
	/* FIXME Here include all is common to many interfaces */
};

#endif /* __PPSI_PP_INSTANCE_H__ */
