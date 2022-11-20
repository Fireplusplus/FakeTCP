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
	
	if ((len & 0x01) != 0) {
		buf[len] = 0;
		sz = len / 2 + 1;
	} else {
		sz = len / 2;
	}

	uint32_t sum = 0;
	int i = 0;
	while (i < sz) {
		if ((sum & 0xffff0000) != 0) {
			sum = ((sum & 0xffff0000) >> 16) + (sum & 0x0000ffff);
		} else {
			sum += data[i++];
		}
	}
	while ((sum & 0xffff0000) != 0) {
		sum = ((sum & 0xffff0000) >> 16) + (sum & 0x0000ffff);
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

char* ParseIpPkt(char *buf, ssize_t len, struct pkt_info *info) {
	struct ip *ip_hdr = (struct ip*)buf;
	if (len < sizeof(*ip_hdr)) {
		printf("[Invalid IP], len[%ld] < size[%ld]", len, sizeof(*ip_hdr));
		return NULL;
	}

	int hdr_len = ip_hdr->ip_hl * 4;
	int tol_len = ntohs(ip_hdr->ip_len);

	printf("[IP]: ver: %d, hdr_len: %d, tos: %d, tol_len: %d\n"
		   "id: %d, off: %d\n"
		   "ttl: %d, proto: %d, check_sum: 0x%x\n"
		   "src: %s, dst: %s\n",
		   ip_hdr->ip_v, hdr_len, ip_hdr->ip_tos, tol_len,
		   ip_hdr->ip_id, ip_hdr->ip_off,
		   ip_hdr->ip_ttl, ip_hdr->ip_p, ntohs(ip_hdr->ip_sum),
		   inet_ntoa(ip_hdr->ip_src), inet_ntoa(ip_hdr->ip_dst));
	
	if (info) {
		info->proto = ip_hdr->ip_p;
		memcpy(&info->ip_src, &ip_hdr->ip_src, sizeof(info->ip_src));
		memcpy(&info->ip_dst, &ip_hdr->ip_dst, sizeof(info->ip_dst));
	}

	return (char*)(buf + hdr_len);  // 返回传输层包头
}

char* ParseTcpPkt(char *buf, ssize_t len, struct pkt_info *info) {
	struct tcphdr *tcp_hdr = (struct tcphdr*)(buf);
	if (len < sizeof(*tcp_hdr)) {
		printf("[Invalid tcp], len[%ld] < sizeof(*tcp_hdr)[%ld]\n", len, sizeof(*tcp_hdr));
		return NULL;
	}

	int off = tcp_hdr->doff * 4;
	printf("[TCP]: sport: %d, dport: %d\n"
	       "seq: %u\n"
		   "ack_seq: %u\n"
		   "off: %d, urg: %d, ack: %d, psh: %d, rst: %d, syn: %d, fin: %d, window: %d\n"
		   "check: %d\n",
		   ntohs(tcp_hdr->source), ntohs(tcp_hdr->dest),
		   ntohl(tcp_hdr->seq), ntohl(tcp_hdr->ack_seq),
		   off, tcp_hdr->urg, tcp_hdr->ack,
		   tcp_hdr->psh, tcp_hdr->rst, tcp_hdr->syn, tcp_hdr->fin, tcp_hdr->window,
		   tcp_hdr->check);

	if (info) {
		info->port_src = ntohs(tcp_hdr->source);
		info->port_dst = ntohs(tcp_hdr->dest);
		info->seq = ntohl(tcp_hdr->seq);
		info->ack_seq = ntohl(tcp_hdr->ack_seq);
		info->syn = tcp_hdr->syn;
		info->ack = tcp_hdr->ack;
		info->rst = tcp_hdr->rst;
	}

	return (char*)(buf + off);  // 返回应用层头
}

char* ParsePkt(char *buf, ssize_t len, struct pkt_info *info) {
	assert(info);
	char *trans_hdr = ParseIpPkt(buf, len, info);
	if (!trans_hdr) return NULL;

	if (info->proto == IPPROTO_TCP) {
		ssize_t trans_len = len - (trans_hdr - buf);
		char *app_layer = ParseTcpPkt(trans_hdr, trans_len, info);
		return app_layer;
	}

	return NULL;
}

int BuildIpPkt(char* data, int len, struct in_addr *ip_src, struct in_addr *ip_dst, uint8_t proto) {
	struct ip *ip_hdr = (struct ip*)(data - sizeof(struct ip));

	memcpy(&ip_hdr->ip_src, ip_src, sizeof(*ip_src));
	memcpy(&ip_hdr->ip_dst, ip_dst, sizeof(*ip_dst));
	ip_hdr->ip_v = 4;
	ip_hdr->ip_hl = sizeof(*ip_hdr) / 4;
	ip_hdr->ip_len = htons(len + sizeof(*ip_hdr));
	ip_hdr->ip_off = htons(0);
	ip_hdr->ip_ttl = 0xff;
	ip_hdr->ip_p = proto;
	ip_hdr->ip_id = 0;
	ip_hdr->ip_sum = htons(GetIpCheckSum((char*)ip_hdr, sizeof(*ip_hdr)));
	
	return sizeof(struct ip) + len;
}

int BuildPkt(char* data, int len,
			 struct in_addr *ip_src, short port_src,
			 struct in_addr *ip_dst, short port_dst,
			 uint32_t seq, uint32_t ack_seq, uint8_t syn, uint8_t ack) {
	struct tcphdr *tcp_hdr = (struct tcphdr*)(data - sizeof(struct tcphdr));

	tcp_hdr->doff = sizeof(*tcp_hdr) / 4;
	tcp_hdr->source = htons(port_src);
	tcp_hdr->dest = htons(port_dst);
	tcp_hdr->seq = htonl(seq);
	tcp_hdr->ack_seq = htonl(ack_seq);
	if (syn) tcp_hdr->syn = 1;
	if (ack) tcp_hdr->ack = 1;
	tcp_hdr->th_sum = GetTcpCheckSum((char*)tcp_hdr, sizeof(*tcp_hdr) + len, ip_src, ip_dst);

	return BuildIpPkt((char*)tcp_hdr, len + sizeof(struct tcphdr), ip_src, ip_dst, IPPROTO_TCP);
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

ssize_t recv_from_addr(int sockfd, void *buf, size_t len, int flags,
                       struct sockaddr_in *src_addr, socklen_t *addrlen, struct pkt_info *info) {
	assert(src_addr && addrlen);
	uint16_t port_src = src_addr->sin_port;	
	ssize_t sz;

	do {
		sz = recvfrom(sockfd, buf, len, flags, (struct sockaddr *)src_addr, addrlen);
		if (sz <= 0) {
			perror("recvfrom");
			return sz;
		}
		ParsePkt(buf, sz, info);
		if (info->port_src != port_src) {
			continue;
		}
		return sz;
	} while (1);
	return 0;
}




