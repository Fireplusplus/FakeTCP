#include <stdio.h>
#include <assert.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "fake_tcp.h"

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) < (b) ? (b) : (a))	


#define __LOG(level, fmt, args...)		do {	\
			printf(level "|%s:%d|%s|" fmt "\n", \
			__FILE__, \
			__LINE__, __FUNCTION__, ## args);	\
		} while (0)

#define DEBUG(fmt, args...)		\
		__LOG("[debug]", fmt, ## args)
#define INFO(fmt, args...)		\
		__LOG("[info]", fmt, ## args)
#define WARN(fmt, args...)		\
		__LOG("[warn]", fmt, ## args)
#define ERROR(fmt, args...)		\
		__LOG("[error]", fmt, ## args)

#define DUMP_HEX(str, data, len)	do {					\
		char tmp[2048], *p = tmp;							\
		uint32_t __len = MIN((uint32_t)sizeof(tmp), (uint32_t)len);		\
		for (uint32_t i = 0; i < __len; i++) {				\
			if (i && i % 8 == 0) 							\
				p += snprintf(p, sizeof(tmp) - (p - tmp) - 1, "\n");						\
			p += snprintf(p, sizeof(tmp) - (p - tmp) - 1, "%02x ", ((uint8_t*)data)[i]);	\
		}	\
		DEBUG("%s ==> len:%d:\n%s", (const char*)str, __len, tmp);	\
	} while (0)


char* ReserveHdrSize(char *buf) {
	int size = sizeof(struct ip) + sizeof(struct tcphdr);
	memset(buf, 0, size);
	return buf + size;
}

uint16_t GetCheckSum(char *buf, int len) {
	uint16_t *data = (uint16_t*)buf;
	int sz;
	
	if (len & 0x01) {
		buf[len] = 0;
		sz = len / 2 + 1;
	} else {
		sz = len / 2;
	}

	uint32_t sum = 0;
	int i = 0;
	while (i < sz) {
		if (sum & 0xffff0000) {
			sum = (sum & 0xffff0000) + (sum & 0x0000ffff);
		} else {
			sum += data[i++];
		}
	}
	while (sum & 0xffff0000) {
		sum = (sum & 0xffff0000) + (sum & 0x0000ffff);
	}

	return ~sum;
}

struct check_sum_hdr {
	struct in_addr ip_src;
	struct in_addr ip_dst;
	uint8_t zero;
	uint8_t proto;
	short tcp_len;
};

uint16_t GetTcpCheckSum(char *buf, int len, struct in_addr* ip_src, struct in_addr* ip_dst) {
	struct check_sum_hdr *sum_hdr = (struct check_sum_hdr*)(buf - sizeof(struct check_sum_hdr));
	memset(sum_hdr, 0, sizeof(struct check_sum_hdr));

	memcpy(&sum_hdr->ip_src, ip_src, sizeof(struct in_addr));
	memcpy(&sum_hdr->ip_dst, ip_dst, sizeof(struct in_addr));
	sum_hdr->proto = IPPROTO_TCP;
	sum_hdr->tcp_len = htons(len);

	return GetCheckSum((char*)sum_hdr, len + sizeof(*sum_hdr));
}

uint16_t GetIpCheckSum(char *buf, int len) {
	return GetCheckSum(buf, len);
}

const char* ParsePkt(const char* buf, ssize_t len) {
	struct ip *ip_hdr = (struct ip*)buf;
	if (len < sizeof(*ip_hdr)) {
		printf("[Invalid IP], len[%ld] < size[%ld]", len, sizeof(*ip_hdr));
		return "";
	}

	int hdr_len = ip_hdr->ip_hl * 4;
	int tol_len = ntohs(ip_hdr->ip_len);
	
	struct tcphdr *tcp_hdr = (struct tcphdr*)(buf + hdr_len);
	if (hdr_len + sizeof(*tcp_hdr) > len) {
		printf("[Invalid tcp], hdr_len[%d] + sizeof(*tcp_hdr)[%ld] > len[%ld]\n",
				 hdr_len, sizeof(*tcp_hdr), len);
		return "";
	}

	printf("[IP]: ver: %d, hdr_len: %d, tos: %d, tol_len: %d\n"
		   "id: %d, off: %d\n"
		   "ttl: %d, proto: %d, check_sum: 0x%x\n"
		   "src: %s, dst: %s\n",
		   ip_hdr->ip_v, hdr_len, ip_hdr->ip_tos, tol_len,
		   ip_hdr->ip_id, ip_hdr->ip_off,
		   ip_hdr->ip_ttl, ip_hdr->ip_p, ntohs(ip_hdr->ip_sum),
		   inet_ntoa(ip_hdr->ip_src), inet_ntoa(ip_hdr->ip_dst));

	int off = tcp_hdr->doff * 4;
	printf("[TCP]: sport: %d, dport: %d\n"
	       "seq: %d\n"
		   "ack_seq: %d\n"
		   "off: %d, urg: %d, ack: %d, psh: %d, rst: %d, syn: %d, fin: %d, window: %d\n"
		   "check: %d\n",
		   ntohs(tcp_hdr->source), ntohs(tcp_hdr->dest),
		   ntohl(tcp_hdr->seq), ntohl(tcp_hdr->ack_seq),
		   off, tcp_hdr->urg, tcp_hdr->ack,
		   tcp_hdr->psh, tcp_hdr->rst, tcp_hdr->syn, tcp_hdr->fin, tcp_hdr->window,
		   tcp_hdr->check);

	return (buf + hdr_len + off);
}

int BuildPkt(char* data, int len,
 			 struct in_addr *ip_src, short port_src, struct in_addr *ip_dst, short port_dst,
			 uint32_t ack_seq) {
	/*if (len < 64) {
		memset(data + len, 0, 64 - len);
		len = 64;
	} else if ((len & 0x01)) {
		memset(data + len, 0, 1);
		len++;
	}*/
	struct tcphdr *tcp_hdr = (struct tcphdr*)(data - sizeof(struct tcphdr));
	tcp_hdr->doff = sizeof(*tcp_hdr) / 4;
	tcp_hdr->source = htons(port_src);
	tcp_hdr->dest = htons(port_dst);
	tcp_hdr->seq = htonl(0);
	tcp_hdr->ack_seq = htonl(0);
	tcp_hdr->th_sum = GetTcpCheckSum((char*)tcp_hdr, sizeof(*tcp_hdr) + len, ip_src, ip_dst);

	struct ip *ip_hdr = (struct ip*)(data - sizeof(struct ip) - sizeof(struct tcphdr));
	memcpy(&ip_hdr->ip_src, ip_src, sizeof(*ip_src));
	memcpy(&ip_hdr->ip_dst, ip_dst, sizeof(*ip_dst));
	ip_hdr->ip_v = 4;
	ip_hdr->ip_hl = sizeof(*ip_hdr) / 4;
	ip_hdr->ip_len = htons(sizeof(*tcp_hdr) + len + sizeof(*ip_hdr));
	ip_hdr->ip_off = htons(0);
	ip_hdr->ip_ttl = 0xff;
	ip_hdr->ip_p = IPPROTO_TCP;
	ip_hdr->ip_id = 0;
	ip_hdr->ip_sum = htons(GetIpCheckSum((char*)ip_hdr, sizeof(*ip_hdr)));
	
	return sizeof(struct ip) + sizeof(struct tcphdr) + len;
}

void SetAddr(const char *ip, short port, struct sockaddr_in* addr) {
	assert(addr);
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(ip);
};

int StartFakeTcp(const char *ip, short port) {
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	const int on =1;
	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("setsockopt");
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



