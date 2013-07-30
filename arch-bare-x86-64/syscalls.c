/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

int bare_errno;

/* This function from uClibc::libc/sysdeps/linux/x86_6/__syscall_error.c */

/* Wrapper for setting errno.
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <errno.h>
#include <features.h>

/* This routine is jumped to by all the syscall handlers, to stash
 * an error number into errno.  */
int __syscall_error(void)
{
	register int err_no __asm__ ("%rcx");
	__asm__ ("mov %rax, %rcx\n\t"
		 "neg %rcx");
	bare_errno = err_no; /* changed for ptp-proposal/proto */
	return -1;
}

/* end of copy from libc/sysdeps/linux/x86_6/__syscall_error.c */

#include <linux/unistd.h>

#include <ppsi/ppsi.h>
#include "bare-linux.h"

/*
 * We depends on syscall.S that does the register passing
 * Usage: long syscall (syscall_number, arg1, arg2, arg3, arg4, arg5, arg6)
 */
extern long syscall(uint64_t n, uint64_t arg1, uint64_t arg2, uint64_t arg3,
		    uint64_t arg4, uint64_t arg5, uint64_t arg6);

int write(int fd, const void *buf, int count)
{
	return syscall(__NR_write, (uint64_t)fd, (uint64_t)buf, (uint64_t)count,
		       0, 0, 0);
}

int exit(int exitcode)
{
	return syscall(__NR_exit, (uint64_t)exitcode, 0, 0,
		       0, 0, 0);
}

int time(long *t)
{
	return syscall(__NR_time, (uint64_t)t, 0, 0,
		       0, 0, 0);
}

int ioctl(int fd, int cmd, void *arg)
{
	return syscall(__NR_ioctl, (uint64_t)fd, (uint64_t)cmd, (uint64_t)arg,
			0, 0, 0);
}

/*
 * In the bare arch I'd better use sys_ prefixed names
 */
int sys_write(int fd, const void *buf, int count)
	__attribute__((alias("write")));
void sys_exit(int exitval)
	__attribute__((alias("exit")));
int sys_time(int tz)
	__attribute__((alias("time")));
int sys_ioctl(int fd, int cmd, void *arg)
	__attribute__((alias("ioctl")));

int sys_select(int max, void *in, void *out, void *exc, void *tout)
{
	return syscall(__NR_select, (uint64_t)max, (uint64_t)in, (uint64_t)out,
			(uint64_t) exc, (uint64_t)tout, 0);
}

int sys_socket(int domain, int type, int proto)
{
	return syscall(__NR_socket, (uint64_t)domain, (uint64_t)type,
			(uint64_t)proto, 0, 0, 0);
}

int sys_bind(int fd, const struct bare_sockaddr *addr, int addrlen)
{
	return syscall(__NR_bind, (uint64_t)fd, (uint64_t)addr,
			(uint64_t)addrlen, 0, 0, 0);
}

int sys_recv(int fd, void *pkt, int plen, int flags)
{
	return syscall(__NR_recvfrom, (uint64_t)fd, (uint64_t)pkt,
			(uint64_t)plen, (uint64_t)flags, 0, 0);
}

int sys_send(int fd, void *pkt, int plen, int flags)
{
	return syscall(__NR_sendto, (uint64_t)fd, (uint64_t)pkt,
			(uint64_t)plen, (uint64_t)flags, 0, 0);
}

int sys_setsockopt(int fd, int level, int optname, const void *optval,
								int optlen)
{
	return syscall(__NR_setsockopt, (uint64_t)fd, (uint64_t)level,
			(uint64_t)optname, (uint64_t)optval, optlen, 0);
}

int sys_close(int fd)
{
	return syscall(__NR_close, (uint64_t)fd, 0,
			0, 0, 0, 0);
}

int sys_shutdown(int fd, int flags)
{
	return syscall(__NR_shutdown, (uint64_t)fd, (uint64_t)flags,
			0, 0, 0, 0);
}

int sys_gettimeofday(void *tv, void *z)
{
	return syscall(__NR_gettimeofday, (uint64_t)tv, (uint64_t)z,
			0, 0, 0, 0);
}
int sys_settimeofday(void *tv, void *z)
{
	return syscall(__NR_settimeofday, (uint64_t)tv, (uint64_t)z,
			0, 0, 0, 0);
}
int sys_adjtimex(void *tv)
{
	return syscall(__NR_adjtimex, (uint64_t)tv, 0,
			0, 0, 0, 0);
}
int sys_clock_gettime(int clock, void *t)
{
	return syscall(__NR_clock_gettime, (uint64_t)clock, (uint64_t)t,
			0, 0, 0, 0);
}
