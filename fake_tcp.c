#include <stdio.h>
#include <assert.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


#include "fake_tcp.h"

void ParsePkt(const char* buf, ssize_t len) {
	struct ip *ip_hdr = (struct ip*)buf;
	if (len < sizeof(*ip_hdr)) {
		printf("[Invalid IP], len[%ld] < size[%ld]", len, sizeof(*ip_hdr));
		return;
	}

	int hdr_len = ip_hdr->ip_hl * 4;
	/*printf("[IP]: ver: %d, hdr_len: %d, tos: %d, tol_len: %d\n"
		   "id: %d, off: %d\n"
		   "ttl: %d, proto: %d, check_sum: %d\n"
		   "src: %s, dst: %s\n",
		   ip_hdr->ip_v, hdr_len, ip_hdr->ip_tos, ip_hdr->ip_len,
		   ip_hdr->ip_id, ip_hdr->ip_off,
		   ip_hdr->ip_ttl, ip_hdr->ip_p, ip_hdr->ip_sum,
		   inet_ntoa(ip_hdr->ip_src), inet_ntoa(ip_hdr->ip_dst));*/
	
	struct tcphdr *tcp_hdr = (struct tcphdr*)(buf + hdr_len);
	if (hdr_len + sizeof(*tcp_hdr) < len) {
		//printf("[Invalid tcp], hdr_len[%d] + sizeof(*tcp_hdr)[%ld] < len[%ld]\n",
		//		 hdr_len, sizeof(*tcp_hdr), len);
		return;
	}

if (tcp_hdr->source != 6666) {
	return;
}
	printf("[TCP]: sport: %x, dport: %x\n"
	       "seq: %d\n"
		   "ack_seq: %d\n"
		   "off: %d, urg: %d, ack: %d, psh: %d, rst: %d, syn: %d, fin: %d, window: %d\n"
		   "check: %d\n",
		   tcp_hdr->source, tcp_hdr->dest,
		   tcp_hdr->seq, tcp_hdr->ack_seq,
		   tcp_hdr->doff, tcp_hdr->urg, tcp_hdr->ack,
		   tcp_hdr->psh, tcp_hdr->rst, tcp_hdr->syn, tcp_hdr->fin, tcp_hdr->window,
		   tcp_hdr->check);
}

void SetAddr(const char *ip, short port, struct sockaddr_in* addr) {
	assert(addr);
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(ip);
}

int StartFakeTcp(const char *ip, short port) {
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	struct sockaddr_in local;
	SetAddr(ip, port, &local);

	if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		return -1;
	}

	return sock;
}

int StartServer(const char *ip, short port) {
	int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock < 0) {
		perror("socket");
		return -1;
	}

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr(ip);

	if (bind(listen_sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(listen_sock, 5) < 0) {
		perror("listen");
		return -1;
	}

	struct sockaddr_in peer;
	socklen_t len = sizeof(peer);	
	int fd = accept(listen_sock, (struct sockaddr *)&peer, &len);
	if (fd < 0) {
		perror("accept");
	}

	printf("new connect: %s:%d\n", inet_ntoa(peer.sin_addr), peer.sin_port);
	close(listen_sock);
	return fd;
}

int StartClient(const char *ip, short port) {
	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	struct sockaddr_in remote;
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	remote.sin_addr.s_addr = inet_addr(ip);

	if (connect(fd, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
		perror("connect");
		return -1;
	}

	return fd;
}

void Stop(int fd) {
	close(fd);
}



