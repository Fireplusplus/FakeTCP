#include "fake_tcp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

static const char *s_ip = "127.0.0.1";
static short s_port = 6666;
static const char *c_ip = "127.0.0.1";
static short c_port = 6667;

void ServerRun(int fd)
{
	char buf[65535];
	// static int msg_cnt = 0;

	while (1)
	{
		struct sockaddr_in from_addr;
		socklen_t from_size;
		ssize_t sz = recvfrom(fd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from_addr, &from_size);
		if (sz < 0)
		{
			perror("recvfrom");
			break;
		}
		else if (0 == sz)
		{
			printf("recvfrom done...\n");
			break;
		}
		else
		{
			buf[sz - 1] = '\0';
			const char *str = ParsePkt(buf, sz);
			if (strlen(str) == 0)
			{
				continue;
			}
			printf("client# [%ld]%s\n", strlen(str), str);

			// snprintf(buf, sizeof(buf), "msg cnt: %d\n", ++msg_cnt);
			// sz = sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&from_addr, from_size);
			/*if (sz < 0) {
				perror("sendto");
				break;
			}*/
		}
	}
}

void ClientRun(int fd)
{
	char buf[65535] = {0};
	struct sockaddr_in server_addr;
	SetAddr(s_ip, s_port, &server_addr);

	struct in_addr client_in_addr, server_in_addr;
	inet_aton(c_ip, &client_in_addr);
	inet_aton(c_ip, &server_in_addr);

	char *data = ReserveHdrSize(buf);

	while (1)
	{
		printf("Please Enter# ");
		fflush(stdout);
		ssize_t _s = read(0, data, sizeof(buf) - 1 - (data - buf));
		if (_s < 0)
		{
			break;
		}
		data[_s] = '\0';

		int total = BuildPkt(data, _s, &client_in_addr, c_port, &server_in_addr, s_port, 0);
		_s = sendto(fd, buf, total, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if (_s < 0)
		{
			perror("sendto");
			break;
		}
		printf("send size: %d\n", (int)_s);

		/*struct sockaddr_in from_addr;
		socklen_t from_size;
		ssize_t sz = recvfrom(fd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from_addr, &from_size);
		if (sz < 0) {
			perror("recvfrom");
			break;
		} else if (sz == 0) {
			printf("recvfrom done...");
			break;
		} else {
			buf[sz] = '\0';
			printf("server echo# %s\n", buf);
		}*/
	}
}

// TODO 伪装tcp发送握手与真实tcp建立连接
int main(int argc, char *argv[])
{
	int server = 0;
	if (argc >= 2 && strncmp(argv[1], "-s", 2) == 0)
	{
		server = 1;
	}

	printf("run in %s mode...\n", server ? "server" : "client");

	// int fd = server ? StartServer(s_ip, s_port) : StartClient(s_ip, s_port);
	// 原始套接字不需绑定端口号
	int fd = server ? StartServer(s_ip, s_port) : StartFakeTcp(c_ip, c_port);
	if (fd < 0)
	{
		return -1;
	}

	if (server)
	{
		ServerRun(fd);
	}
	else
	{
		ClientRun(fd);
	}

	Stop(fd);
	return 0;
}
