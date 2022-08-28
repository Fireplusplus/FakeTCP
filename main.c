#include "fake_tcp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *s_ip = "127.0.0.1";
static short s_port = 6666;

void ServerRun(int fd) {
	char buf[1024];
	memset(buf, '\0', sizeof(buf));
	static int msg_cnt = 0;

	while (1)
	{
		ssize_t sz = read(fd, buf, sizeof(buf) - 1);
		if (sz < 0) {
			perror("read");
			break;
		} else if (0 == sz) {
			printf("read done...\n");
			break;
		} else {
			buf[sz - 1] = '\0';
			printf("client# %s\n", buf);

			snprintf(buf, sizeof(buf), "msg cnt: %d\n", msg_cnt);
			write(fd, buf, strlen(buf));
		}
	}
}

void ClientRun(int fd) {
	char buf[1024];
	memset(buf, '\0', sizeof(buf));
	
	while (1)
	{
		printf("Please Enter# ");
		fflush(stdout);
		ssize_t _s = read(0, buf, sizeof(buf) - 1);
		if (_s > 0) {
			buf[_s] = '\0';
		}
		_s = write(fd, buf, strlen(buf));
		if (_s < 0) {
			perror("write");
		}

		ssize_t sz = read(fd, buf, sizeof(buf) - 1);
		if (sz < 0) {
			perror("read");
			break;
		} else if (sz == 0) {
			printf("read done...");
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

	printf("run in %s mode...", server ? "server" : "client");

	int fd = server ? StartServer(s_ip, s_port) : StartClient(s_ip, s_port);
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

