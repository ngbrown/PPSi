/*
 * This is the shared memory interface for multi-process cooperation
 * within the whiterabbit switch. Everyone exports status information.
 */
#ifndef __WRS_SHM_H__
#define __WRS_SHM_H__
#include <stdint.h>

#define WRS_SHM_FILE  "/dev/shm/wrs-shmem"
#define WRS_SHM_SIZE  (64*1024) /* each */

/* Every process gets 8 pages (32k) to be safe for the future */
enum wrs_shm_name {
	wrs_shm_ptp,
	wrs_shm_rtu,
	wrs_shm_hal,
	wrs_shm_vlan,
	WRS_SHM_N_NAMES,	/* must be last */
};

/* Each area starts with this process identifier */
struct wrs_shm_head {
	void *mapbase;		/* In writer's addr space (to track ptrs) */
	char name[7 * sizeof(void *)];

	unsigned long stamp;	/* Last modified, w/ CLOCK_MONOTONIC */
	unsigned long data_off;	/* Where the structure lives */
	int shm_name;		/* The enum above, for cross-checking */
	int pid;		/* The current pid owning the area */

	unsigned pidsequence;	/* Each new pid must increments this */
	unsigned sequence;	/* If we need consistency, this is it */
	unsigned version;	/* Version of the data structure */
	unsigned data_size;	/* Size of it (for binary dumps) */
};

/* flags */
#define WRS_SHM_READ  0x0000
#define WRS_SHM_WRITE 0x0001

/* get vs. put, like in the kernel. Errors are in errno (see source) */
void *wrs_shm_get(enum wrs_shm_name name_id, char *name, unsigned long flags);
int wrs_shm_put(void *headptr);

/* The writer can allocate structures that live in the area itself */
void *wrs_shm_alloc(void *headptr, size_t size);

/* The reader can track writer's pointers, if they are in the area */
void *wrs_shm_follow(void *headptr, void *ptr);

/* Before and after writing a chunk of data, act on sequence and stamp */
extern void wrs_shm_write(void *headptr, int begin);

/* A reader can rely on the sequence number (in the <linux/seqlock.h> way) */
extern unsigned wrs_shm_seqbegin(void *headptr);
extern int wrs_shm_seqretry(void *headptr, unsigned start);

/* A reader can check wether information is current enough */
extern int wrs_shm_age(void *headptr);

/* A reader can get the information pointer, for a specific version, or NULL */
extern void *wrs_shm_data(void *headptr, unsigned version);

#endif /* __WRS_SHM_H__ */
