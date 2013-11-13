/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

/*
 * These are the functions provided by the various unix files
 */

#define POSIX_ARCH(ppg) ((struct unix_arch_data *)(ppg->arch_data))
struct unix_arch_data {
	struct timeval tv;
};

extern void unix_main_loop(struct pp_globals *ppg);
