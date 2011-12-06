/*
 * FIXME header
 */

/* Timer interface for GNU/Linux (and most likely other posix systems */

#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include <pproto/pproto.h>
#include <pproto/diag.h>
#include "posix.h"

extern int posix_timer_init(struct pp_timer *tm)
{
	/* TODO */
	return 0;
}

extern int posix_timer_start(struct pp_timer *tm)
{
	/* TODO */
	return 0;
}

extern int posix_timer_stop(struct pp_timer *tm)
{
	/* TODO */
	return 0;
}

extern int posix_timer_update(struct pp_timer *tm)
{
	/* TODO */
	return 0;
}

extern int posix_timer_expired(struct pp_timer *tm)
{
	/* TODO */
	return 0;
}


int pp_timer_init(struct pp_timer *tm)
	__attribute__((alias("posix_timer_init")));

int pp_timer_start(struct pp_timer *tm)
	__attribute__((alias("posix_timer_start")));

int pp_timer_stop(struct pp_timer *tm)
	__attribute__((alias("posix_timer_stop")));

int pp_timer_update(struct pp_timer *tm)
	__attribute__((alias("posix_timer_update")));

int pp_timer_expired(struct pp_timer *tm)
	__attribute__((alias("posix_timer_expired")));
