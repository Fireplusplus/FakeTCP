#ifndef __FAKE_TCP_20220828__
#define __FAKE_TCP_20220828__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

struct pkt_info {
	uint8_t proto;
	uint8_t syn:1;
	uint8_t ack:1;
	uint8_t rst:1;
	uint16_t port_src;
	uint16_t port_dst;
	uint32_t seq;
	uint32_t ack_seq;
	struct in_addr ip_src;
	struct in_addr ip_dst;
};

char* ReserveHdrSize(char *buf);

char* ParsePkt(char *buf, ssize_t len, struct pkt_info *info);


int BuildPkt(char* data, int len,
			 struct in_addr *ip_src, short port_src,
			 struct in_addr *ip_dst, short port_dst,
			 uint32_t seq, uint32_t ack_seq, uint8_t syn, uint8_t ack);

void SetAddr(const char *ip, short port, struct sockaddr_in*addr);

int StartFakeTcp(const char *ip, short port);

int StartServer(const char *ip, short port);

int StartClient(const char *ip, short port);

void Stop(int fd);

ssize_t recv_from_addr(int sockfd, void *buf, size_t len, int flags,
                       struct sockaddr_in *src_addr, socklen_t *addrlen, struct pkt_info *info);

#endif
