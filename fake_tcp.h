#ifndef __FAKE_TCP_20220828__
#define __FAKE_TCP_20220828__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

void ParsePkt(const char* buf, ssize_t len);

void SetAddr(const char *ip, short port, struct sockaddr_in*addr);

int StartFakeTcp(const char *ip, short port);

int StartServer(const char *ip, short port);

int StartClient(const char *ip, short port);

void Stop(int fd);

#endif
