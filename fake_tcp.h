#ifndef __FAKE_TCP_20220828__
#define __FAKE_TCP_20220828__

int StartServer(const char *ip, short port);

int StartClient(const char *ip, short port);

void Stop(int fd);

#endif
