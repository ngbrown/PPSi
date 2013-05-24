/*
 * Aurelio Colosimo for CERN, 2013 -- public domain
 */

/* Defined in unix-socket.c */
int unix_net_init(struct pp_instance *ppi);

int unix_net_exit(struct pp_instance *ppi);

int unix_net_recv(struct pp_instance *ppi, void *pkt, int len,
		   TimeInternal *t);

int unix_net_send(struct pp_instance *ppi, void *pkt, int len,
			  TimeInternal *t, int chtype, int use_pdelay_addr);

/* Defined in unix-time.c */
int unix_time_get(struct pp_instance *ppi, TimeInternal *t);

int32_t unix_time_set(struct pp_instance *ppi, TimeInternal *t);

int unix_time_adjust(struct pp_instance *ppi, long offset_ns,
	long freq_ppm);

int unix_time_adjust_offset(struct pp_instance *ppi, long offset_ns);

int unix_time_adjust_freq(struct pp_instance *ppi, long freq_ppm);

unsigned long unix_calc_timeout(struct pp_instance *ppi,
	int millisec);
