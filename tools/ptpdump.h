
int dumpstruct(char *prefix, char *name, void *ptr, int size);
void dump_eth(struct ethhdr *eth);
void dump_ip(struct iphdr *ip);
void dump_udp(struct udphdr *udp);
void dump_1stamp(char *s, struct stamp *t);
void dump_1quality(char *s, ClockQuality *q);
void dump_1clockid(char *s, ClockIdentity i);
void dump_1port(char *s, unsigned char *p);
void dump_msg_announce(struct ptp_announce *p);
void dump_msg_sync_etc(char *s, struct ptp_sync_etc *p);
void dump_msg_resp_etc(char *s, struct ptp_sync_etc *p);
int dump_tlv(struct ptp_tlv *tlv, int totallen);
void dump_payload(void *pl, int len);
int dump_udppkt(void *buf, int len);
int dump_1588pkt(void *buf, int len);

