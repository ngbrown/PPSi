/* Socket interface for Windows systems */


#include <stdlib.h>
#include <errno.h>
#include <Ws2tcpip.h>
#include <Mswsock.h>

#include <Iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#undef TRUE
#undef FALSE

#include <ppsi/ppsi.h>
#include "../arch-win32/ppsi-win32.h"

static LPFN_WSARECVMSG WSARecvMsg;
//static LPFN_WSASENDMSG WSASendMsg;

#define WORKING_BUFFER_SIZE (15*1024)
#define MAX_TRIES 3

// return 0 for orderly shutdown, -1 for error, otherwise, number of bytes.
static int win32_recv_msg(struct pp_instance *ppi, SOCKET fd, void *pkt, int len,
	TimeInternal *t)
{
	int ret;
	WSAMSG msg;
	WSABUF vec[1];
	DWORD numberOfBytesRecvd;

	union {
		WSACMSGHDR cm;
		char control[WSA_CMSG_SPACE(sizeof(struct in_pktinfo))];
	} cmsg_un;

	WSACMSGHDR *cmsg;

	vec[0].buf = pkt;
	vec[0].len = PP_MAX_FRAME_LENGTH;

	memset(&msg, 0, sizeof(msg));
	memset(&cmsg_un, 0, sizeof(cmsg_un));

	/* name, namelen == 0: not used */
	msg.lpBuffers = vec;
	msg.dwBufferCount = 1;
	msg.Control.buf = cmsg_un.control;
	msg.Control.len = sizeof(cmsg_un.control);

	ret = WSARecvMsg(fd, &msg, &numberOfBytesRecvd, NULL, NULL);
	if (ret != 0) {
		int error = WSAGetLastError();
		if (error == WSAETIMEDOUT || error == WSAEINTR)
			return 0;

		return -1;
	}

	if (msg.dwFlags & MSG_TRUNC) {
		/* If we are in VLAN mode, we get everything. This is ok */
		if (ppi->proto != PPSI_PROTO_VLAN)
			pp_error("%s: truncated message\n", __FUNCTION__);
		return -2; /* like "dropped" */
	}

	for (cmsg = WSA_CMSG_FIRSTHDR(&msg); cmsg != NULL;
		cmsg = WSA_CMSG_NXTHDR(&msg, cmsg)) {

		if (cmsg->cmsg_level == IPPROTO_IP &&
			cmsg->cmsg_type == IP_PKTINFO) {

		}
	}

	/*
	* Windows doesn't support capturing packet time
	* get the recording time here, even though it may  put a big
	* spike in the offset signal sent to the clock servo
	*/
	ppi->t_ops->get(ppi, t);

	ppi->peer_vid = 0;

	if (ppsi_drop_rx()) {
		pp_diag(ppi, frames, 1, "Drop received frame\n");
		return -2;
	}

	/* This is not really hw... */
	pp_diag(ppi, time, 2, "recv stamp: %i.%09i (%s)\n",
		(int)t->seconds, (int)t->nanoseconds, "user");
	return numberOfBytesRecvd;
}

/* Receive and send is *not* so trivial */
static int win32_net_recv(struct pp_instance *ppi, void *pkt, int len,
	TimeInternal *t)
{
	struct pp_channel *ch1, *ch2;
	int ret;

	if (ppi->proto == PPSI_PROTO_UDP) {
		/* we can return one frame only, always handle EVT msgs
		* before GEN */
		ch1 = &(ppi->ch[PP_NP_EVT]);
		ch2 = &(ppi->ch[PP_NP_GEN]);

		ret = -1;
		if (ch1->pkt_present)
			ret = win32_recv_msg(ppi, ch1->fd, pkt, len, t);
		else if (ch2->pkt_present)
			ret = win32_recv_msg(ppi, ch2->fd, pkt, len, t);
		if (ret <= 0)
			return ret;
		/* We can't save the peer's mac address in UDP mode */
		if (pp_diag_allow(ppi, frames, 2))
			dump_payloadpkt("recv: ", pkt, ret, t);
		return ret;
	}

	// Don't handle RAW, VLAN
	pp_diag(ppi, frames, 1, "win32_net_recv not UDP\n");
	return -1;
}

static int win32_net_send(struct pp_instance *ppi, void *pkt, int len,
	TimeInternal *t, int chtype, int use_pdelay_addr)
{
	struct sockaddr_in addr;
	static uint16_t udpport[] = {
		[PP_NP_GEN] = PP_GEN_PORT,
		[PP_NP_EVT] = PP_EVT_PORT,
	};
	int ret;

	/* To fake a network frame loss, set the timestamp and do not send */
	if (ppsi_drop_tx()) {
		if (t)
			ppi->t_ops->get(ppi, t);
		pp_diag(ppi, frames, 1, "Drop sent frame\n");
		return len;
	}

	if (ppi->proto == PPSI_PROTO_UDP) {
		addr.sin_family = AF_INET;
		addr.sin_port = htons(udpport[chtype]);
		addr.sin_addr.s_addr = ppi->mcast_addr;

		if (t)
			ppi->t_ops->get(ppi, t);

		ret = sendto(ppi->ch[chtype].fd, pkt, len, 0,
			(struct sockaddr *)&addr,
			sizeof(struct sockaddr_in));
		if (pp_diag_allow(ppi, frames, 2))
			dump_payloadpkt("send: ", pkt, len, t);
		return len;
	}

	// Don't handle RAW, VLAN
	pp_diag(ppi, frames, 1, "win32_net_send not UDP\n");
	return -1;
}

static int win32_find_interface(struct pp_instance *ppi, const char *ifname, unsigned char *hwaddr, PIN_ADDR p_iface_addr)
{
	DWORD dwRetVal = 0;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	ULONG outBufLen = 0;
	ULONG Iterations = 0;
	char addr_str[INET_ADDRSTRLEN];
	char friendlyName[MAX_ADAPTER_NAME_LENGTH];
	PIP_ADAPTER_UNICAST_ADDRESS pUnicastAddress;
	int result = -1;

	PIP_ADAPTER_ADDRESSES pMatchingAddress = NULL;

	// Allocate a 15 KB buffer to start with.
	outBufLen = WORKING_BUFFER_SIZE;

	do {
		pAddresses = (IP_ADAPTER_ADDRESSES *)HeapAlloc(GetProcessHeap(), 0, (outBufLen));

		if (pAddresses == NULL) {
			pp_printf("%s: Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n", __FUNCTION__);
			return -1;
		}

		dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST, NULL, pAddresses, &outBufLen);

		if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
			HeapFree(GetProcessHeap(), 0, (pAddresses));
			pAddresses = NULL;
		} else {
			break;
		}

	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

	if (dwRetVal == NO_ERROR) {
		pCurrAddresses = pAddresses;
		while (pCurrAddresses) {

			pUnicastAddress = pCurrAddresses->FirstUnicastAddress;
			if (pUnicastAddress != NULL){
				for (size_t i = 0; pUnicastAddress != NULL; i++) {
					if (pUnicastAddress->Address.lpSockaddr->sa_family == AF_INET) {
						break;
					}

					pUnicastAddress = pUnicastAddress->Next;
				}
			}
			if (pUnicastAddress != NULL && pUnicastAddress->Address.lpSockaddr->sa_family == AF_INET){
				PIN_ADDR p_sin_addr = &((PSOCKADDR_IN)pUnicastAddress->Address.lpSockaddr)->sin_addr;
				wcstombs(friendlyName, pCurrAddresses->FriendlyName, sizeof(friendlyName));
				pp_diag(ppi, frames, 2, "Found Adaptor [%u]: %s, %s\n", pCurrAddresses->IfIndex, friendlyName, inet_ntop(AF_INET, p_sin_addr, addr_str, INET_ADDRSTRLEN));

				if (_stricmp(friendlyName, ifname) == 0) {
					pMatchingAddress = pCurrAddresses;
					memcpy(hwaddr, pCurrAddresses->PhysicalAddress, min(6, pCurrAddresses->PhysicalAddressLength));
					memcpy(p_iface_addr, p_sin_addr, sizeof(IN_ADDR));
					pp_diag(ppi, frames, 1, "* Matching Adaptor [%u]: %s, %s\n", pCurrAddresses->IfIndex, friendlyName, inet_ntop(AF_INET, p_sin_addr, addr_str, INET_ADDRSTRLEN));
					result = 0;
				}
			}

			pCurrAddresses = pCurrAddresses->Next;
		}
	} else {
		pp_printf("%s: Call to GetAdaptersAddresses failed with error: %d\n", __FUNCTION__, dwRetVal);
		if (dwRetVal == ERROR_NO_DATA) {
			pp_printf("%s: No addresses were found for the requested parameters\n", __FUNCTION__);
		}

		result = -1;
	}

	if (pAddresses) {
		HeapFree(GetProcessHeap(), 0, (pAddresses));
	}

	return result;
}

static int win32_open_ch_udp(struct pp_instance *ppi, const char *ifname, int chtype)
{
	SOCKET sock = INVALID_SOCKET;
	int temp;
	struct in_addr iface_addr, net_addr;
	struct sockaddr_in addr;
	struct ip_mreq imr;
	char addr_str[INET_ADDRSTRLEN];
	char *context;
	int err;
	WORD wsaVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;

	if (chtype == 0) {
		// win32_open_ch_udp is called twice, but we only need to startup the first time.
		context = "WSAStartup()";
		err = WSAStartup(wsaVersionRequested, &wsaData);
		if (err != 0)
			goto err_out;
	}

	context = "socket()";
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		goto err_out;

	ppi->ch[chtype].fd = sock;

	GUID WSARecvMsg_GUID = WSAID_WSARECVMSG;
	if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
				&WSARecvMsg_GUID, sizeof WSARecvMsg_GUID,
				&WSARecvMsg, sizeof WSARecvMsg,
				&temp, NULL, NULL) != 0)
		goto err_out;

	//GUID WSASendMsg_GUID = WSAID_WSASENDMSG;
	//WSAIoctl(pif->gen_sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
	//	      &WSASendMsg_GUID, sizeof WSASendMsg_GUID,
	//	      &WSASendMsg, sizeof WSASendMsg,
	//	      &numberOfBytes, NULL, NULL);

	/* hw interface information */
	// find hardware address of adaptor matching ifname
	// find source address of same adaptor
	if (win32_find_interface(ppi, ifname, ppi->ch[chtype].addr, &iface_addr) != 0) {
		goto err_out;
	}
	pp_diag(ppi, frames, 2, "Local IP address used : %s\n", inet_ntop(AF_INET, &iface_addr, addr_str, INET_ADDRSTRLEN));

	temp = 1; /* allow address reuse */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) < 0)
		pp_printf("%s: ioctl(SO_REUSEADDR): %s\n", __FUNCTION__, strerror(errno));

	/* bind sockets */
	/* need INADDR_ANY to allow receipt of multi-cast and uni-cast
	* messages */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(chtype == PP_NP_GEN ? PP_GEN_PORT : PP_EVT_PORT);
	context = "bind()";
	if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
		goto err_out;

	/* Init General multicast IP address */
	memcpy(addr_str, PP_DEFAULT_DOMAIN_ADDRESS, INET_ADDRSTRLEN);

	context = addr_str;
	if (inet_pton(AF_INET, addr_str, &net_addr) != 1)
	{
		_set_errno(EINVAL);
		goto err_out;
	}
	ppi->mcast_addr = net_addr.s_addr;

	/* multicast sends only on specified interface */
	imr.imr_multiaddr.s_addr = net_addr.s_addr;
	imr.imr_interface.s_addr = iface_addr.s_addr;
	context = "setsockopt(IP_MULTICAST_IF)";
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &imr.imr_interface.s_addr, sizeof(struct in_addr)) < 0)
		goto err_out;

	/* join multicast group (for recv) on specified interface */
	context = "setsockopt(IP_ADD_MEMBERSHIP)";
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(struct ip_mreq)) < 0)
		goto err_out;
	/* End of General multicast Ip address init */

	/* set socket time-to-live */
	context = "setsockopt(IP_MULTICAST_TTL)";
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &OPTS(ppi)->ttl, sizeof(int)) < 0)
		goto err_out;

	/* forcibly disable loopback */
	temp = 0;
	context = "setsockopt(IP_MULTICAST_LOOP)";
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &temp, sizeof(int)) < 0)
		goto err_out;

	// set the receive timeout to 0
	temp = 0;
	context = "setsockopt(SO_RCVTIMEO)";
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &temp, sizeof(int)) < 0)
		goto err_out;

	ppi->ch[chtype].fd = sock;
	return 0;

err_out:
	pp_printf("%s: %s: errno(%s), wsa_error(0x%x)\n", __FUNCTION__, context, strerror(errno), WSAGetLastError());
	if (sock != INVALID_SOCKET)
		close(sock);
	ppi->ch[chtype].fd = -1;
	return -1;
}

static int win32_net_exit(struct pp_instance *ppi);

/*
* Inits all the network stuff
*/

/* This function must be able to be called twice, and clean-up internally */
static int win32_net_init(struct pp_instance *ppi)
{
	int i;

	if (ppi->ch[0].fd > 0)
		win32_net_exit(ppi);

	/* The buffer is inside ppi, but we need to set pointers and align */
	pp_prepare_pointers(ppi);

	if (ppi->proto == PPSI_PROTO_UDP) {
		if (ppi->nvlans) {
			/* If "proto udp" is set after setting vlans... */
			pp_printf("Error: can't use UDP with VLAN support\n");
			exit(1);
		}

		pp_diag(ppi, frames, 1, "win32_net_init UDP\n");
		for (i = PP_NP_GEN; i <= PP_NP_EVT; i++) {
			if (win32_open_ch_udp(ppi, ppi->iface_name, i))
				return -1;
		}
		return 0;
	}

	// Don't handle RAW, VLAN
	pp_diag(ppi, frames, 1, "win32_net_init not UDP\n");
	return -1;
}

/*
* Shutdown all the network stuff
*/
static int win32_net_exit(struct pp_instance *ppi)
{
	struct ip_mreq imr;
	SOCKET fd;
	int i;

	if (ppi->proto == PPSI_PROTO_UDP) {
		for (i = PP_NP_GEN; i <= PP_NP_EVT; i++) {
			fd = ppi->ch[i].fd;
			if (fd < 0)
				continue;

			/* Close General Multicast */
			imr.imr_multiaddr.s_addr = ppi->mcast_addr;
			imr.imr_interface.s_addr = htonl(INADDR_ANY);

			setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
				&imr, sizeof(struct ip_mreq));
			closesocket(fd);

			ppi->ch[i].fd = -1;
		}

		WSACleanup();

		ppi->mcast_addr = 0;
		return 0;
	}

	return -1;
}

static int win32_net_check_packet(struct pp_globals *ppg, int delay_ms)
{
	fd_set set;
	int i, j, k;
	int ret = 0;
	int maxfd = -1;
	struct win32_arch_data *arch_data = WIN32_ARCH(ppg);
	int old_delay_ms;

	old_delay_ms = arch_data->tv.tv_sec * 1000 +
		arch_data->tv.tv_usec / 1000;

	if ((delay_ms != -1) &&
		((old_delay_ms == 0) || (delay_ms < old_delay_ms))) {
		/* Wait for a packet or for the timeout */
		arch_data->tv.tv_sec = delay_ms / 1000;
		arch_data->tv.tv_usec = (delay_ms - (arch_data->tv.tv_sec * 1000)) * 1000;
	}

	/* Detect general timeout with no needs for select stuff */
	if ((arch_data->tv.tv_sec == 0) && (arch_data->tv.tv_usec == 0))
		return 0;

	FD_ZERO(&set);

	for (j = 0; j < ppg->nlinks; j++) {
		struct pp_instance *ppi = INST(ppg, j);
		int fd_to_set;

		/* Use either fd that is valid, irrespective of ether/udp */
		for (k = 0; k < 2; k++) {
			ppi->ch[k].pkt_present = 0;
			fd_to_set = ppi->ch[k].fd;
			if (fd_to_set < 0)
				continue;

			FD_SET(fd_to_set, &set);
			maxfd = fd_to_set > maxfd ? fd_to_set : maxfd;
		}
	}
	i = select(maxfd + 1, &set, NULL, NULL, &arch_data->tv);

	if (i < 0 && errno != EINTR)
		exit(__LINE__);

	if (i < 0)
		return -1;

	if (i == 0)
		return 0;

	for (j = 0; j < ppg->nlinks; j++) {
		struct pp_instance *ppi = INST(ppg, j);
		int fd = ppi->ch[PP_NP_GEN].fd;

		if (fd >= 0 && FD_ISSET(fd, &set)) {
			ret++;
			ppi->ch[PP_NP_GEN].pkt_present = 1;
		}

		fd = ppi->ch[PP_NP_EVT].fd;

		if (fd >= 0 && FD_ISSET(fd, &set)) {
			ret++;
			ppi->ch[PP_NP_EVT].pkt_present = 1;
		}
	}
	return ret;
}

struct pp_network_operations win32_net_ops = {
	.init = win32_net_init,
	.exit = win32_net_exit,
	.recv = win32_net_recv,
	.send = win32_net_send,
	.check_packet = win32_net_check_packet,
};

