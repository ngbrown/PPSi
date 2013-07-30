/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */
#include <linux/unistd.h>

#include <ppsi/ppsi.h>
#include "bare-linux.h"
#include "syscalls.h"

int bare_errno;

struct sel_arg_struct {
	unsigned long n;
	void *inp, *outp, *exp;
	void *tvp;
};

/*
 * The following lines use defines from Torvalds (linux-2.4.0: see syscalls.h)
 */
_syscall3(int, write, int, fd, const void *, buf, int, count)
_syscall1(int, exit, int, exitcode)
_syscall1(int, time, void *, tz)
_syscall3(int, ioctl, int, fd, int, cmd, void *, arg)
_syscall1(int, select, struct sel_arg_struct *, as)
static _syscall2(int, socketcall, int, call, unsigned long *, args)
_syscall2(int, gettimeofday, void *, tv, void *, z);
_syscall2(int, settimeofday, void *, tv, void *, z);
_syscall1(int, adjtimex, void *, tv);
_syscall2(int, clock_gettime, int, clock, void *, t);
_syscall1(int, close, int, fd);

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
int sys_close(int fd)
	__attribute__((alias("close")));

static struct sel_arg_struct as; /* declared as local, it won't work */
int sys_select(int max, void *in, void *out, void *exc, void *tout)
{
	as.n = max;
	as.inp = in;
	as.outp = out;
	as.exp = exc;
	as.tvp = tout;
	return select(&as);
}
int sys_gettimeofday(void *tv, void *z)
{
	return gettimeofday(tv, z);
}
int sys_settimeofday(void *tv, void *z)
{
	return settimeofday(tv, z);
}
int sys_adjtimex(void *tv)
{
	return adjtimex(tv);
}
int sys_clock_gettime(int clock, void *t)
{
	return clock_gettime(clock, t);
}


/* i386 has the socketcall thing. Bah! */
#define SYS_SOCKET	1		/* sys_socket(2)		*/
#define SYS_BIND	2		/* sys_bind(2)			*/
#define SYS_CONNECT	3		/* sys_connect(2)		*/
#define SYS_LISTEN	4		/* sys_listen(2)		*/
#define SYS_ACCEPT	5		/* sys_accept(2)		*/
#define SYS_GETSOCKNAME 6		/* sys_getsockname(2)		*/
#define SYS_GETPEERNAME 7		/* sys_getpeername(2)		*/
#define SYS_SOCKETPAIR	8		/* sys_socketpair(2)		*/
#define SYS_SEND	9		/* sys_send(2)			*/
#define SYS_RECV	10		/* sys_recv(2)			*/
#define SYS_SENDTO	11		/* sys_sendto(2)		*/
#define SYS_RECVFROM	12		/* sys_recvfrom(2)		*/
#define SYS_SHUTDOWN	13		/* sys_shutdown(2)		*/
#define SYS_SETSOCKOPT	14		/* sys_setsockopt(2)		*/
#define SYS_GETSOCKOPT	15		/* sys_getsockopt(2)		*/
#define SYS_SENDMSG	16		/* sys_sendmsg(2)		*/
#define SYS_RECVMSG	17		/* sys_recvmsg(2)		*/
#define SYS_PACCEPT	18		/* sys_paccept(2)		*/

static unsigned long args[5];

int sys_socket(int domain, int type, int proto)
{
	/*
	 * Strangely, this is not working for me:
	 *     unsigned long args[3] = {domain, type, proto};
	 * So let's use an external thing. Who knows why...
	 */
	args[0] = domain;
	args[1] = type;
	args[2] = proto;
	return socketcall(SYS_SOCKET, args);
}

int sys_bind(int fd, const struct bare_sockaddr *addr, int addrlen)
{
	args[0] = fd;
	args[1] = (unsigned long)addr;
	args[2] = addrlen;
	return socketcall(SYS_BIND, args);
}

int sys_recv(int fd, void *pkt, int plen, int flags)
{
	args[0] = fd;
	args[1] = (unsigned long)pkt;
	args[2] = plen;
	args[3] = flags;
	return socketcall(SYS_RECV, args);
}

int sys_send(int fd, void *pkt, int plen, int flags)
{
	args[0] = fd;
	args[1] = (unsigned long)pkt;
	args[2] = plen;
	args[3] = flags;
	return socketcall(SYS_SEND, args);
}

int sys_shutdown(int fd, int flags)
{
	args[0] = fd;
	args[1] = flags;
	return socketcall(SYS_SHUTDOWN, args);
}

int sys_setsockopt(int fd, int level, int optname, const void *optval,
		   int optlen)
{
	args[0] = fd;
	args[1] = level;
	args[2] = optname;
	args[3] = (unsigned long)optval;
	args[4] = optlen;
	return socketcall(SYS_SETSOCKOPT, args);
}
