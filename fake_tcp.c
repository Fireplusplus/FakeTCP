#include <stdio.h>
#include <assert.h>

#include "fake_tcp.h"

void SetAddr(const char *ip, short port, struct sockaddr_in* addr) {
	assert(addr);
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(ip);
}

int StartFakeTcp(const char *ip, short port) {
	int sock = socket(AF_INET, SOCK_RAW, 0);
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
	int listen_sock = socket(AF_INET, SOCK_RAW, 0);
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
	int fd = socket(AF_INET, SOCK_STREAM, 0);
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



