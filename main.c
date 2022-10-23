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

void ServerRun(int fd) {
	char buf[1024];
	memset(buf, '\0', sizeof(buf));
	static int msg_cnt = 0;

	while (1)
	{
		struct sockaddr_in from_addr;
		socklen_t from_size;
		ssize_t sz = recvfrom(fd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from_addr, &from_size);
		if (sz < 0) {
			perror("recvfrom");
			break;
		} else if (0 == sz) {
			printf("recvfrom done...\n");
			break;
		} else {
			buf[sz - 1] = '\0';
			printf("client# %s\n", buf);

			snprintf(buf, sizeof(buf), "msg cnt: %d\n", ++msg_cnt);
			sz = sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&from_addr, from_size);
			if (sz < 0) {
				perror("sendto");
				break;
			}
		}
	}
}

void ClientRun(int fd) {
	char buf[1024];
	memset(buf, '\0', sizeof(buf));
	struct sockaddr_in server_addr;
	SetAddr(s_ip, s_port, &server_addr);

	while (1)
	{
		printf("Please Enter# ");
		fflush(stdout);
		ssize_t _s = read(0, buf, sizeof(buf) - 1);
		if (_s < 0) {
			break;
		}
		buf[_s] = '\0';

		_s = sendto(fd, buf, _s, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if (_s < 0) {
			perror("sendto");
			break;
		}

		struct sockaddr_in from_addr;
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
		}
	}
}


int main(int argc, char* argv[]) {
	int server = 0;
	if (argc >= 2 && strncmp(argv[1], "-s", 2) == 0) {
		server = 1;
	}

	printf("run in %s mode...\n", server ? "server" : "client");

	//int fd = server ? StartServer(s_ip, s_port) : StartClient(s_ip, s_port);
	int fd = server ? StartFakeTcp(s_ip, s_port) : StartFakeTcp(c_ip, c_port);
	if (fd < 0) {
		return -1;
	}

	if (server) {
		ServerRun(fd);
	} else {
		ClientRun(fd);
	}
	
	Stop(fd);
	return 0;
}

