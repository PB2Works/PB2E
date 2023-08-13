#include <stdio.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include "net.h"
#pragma comment(lib, "ws2_32.lib")

#define MAX_SOCKETS 16
#define BUFFER_SIZE 2048

typedef struct {
	bool active;
	bool connected;
	int s;
	struct sockaddr_in si_local;
	struct sockaddr_in si_server;
	fd_set readfds;
} as3socket;

as3socket sockets[MAX_SOCKETS];
bool initialized = false;
struct timeval select_timeout;
WSADATA wsa;

static void net_setdestinaton(as3socket &sock, const char* ip, int port) {
	sockaddr_in &si = sock.si_server;
	if (sock.connected) return;

	printf("[net_setdestinaton] %s:%d\n", ip, port);

	si.sin_family = AF_INET;
	si.sin_addr.s_addr = inet_addr(ip);
	si.sin_port = htons(port);

	sock.connected = true;
}

static inline void net_freesocket(as3socket& sock) {
	closesocket(sock.s);
}

FREObject AS3_NET_newUDPSocket(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	int id;
	FREObject as3_id;
	as3socket* sock;

	if (!initialized) id = -2;
	else {
		id = -1;

		for (int i = 0; i < MAX_SOCKETS; i++) {
			if (!sockets[i].active) {
				id = i;
				break;
			}
		}

		if (id != -1) {
			sock = &sockets[id];
			sock->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			sock->active = true;
			sock->connected = false;
			FD_ZERO(&sock->readfds);
			FD_SET(sock->s, &sock->readfds);
			printf("[NET] New socket: %d\n", id);
		}
	}
	
	FRENewObjectFromInt32(id, &as3_id);
	return as3_id;
}

FREObject AS3_NET_closeUDPSocket(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	int id;
	FREGetObjectAsInt32(argv[0], &id);

	if (id >= 0 && id < MAX_SOCKETS) {
		sockets[id].active = false;
		net_freesocket(sockets[id]);
	}

	return NULL;
}

FREObject AS3_NET_PollUDP(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	int id;
	as3socket* sock;
	char *buffer;
	sockaddr_in from;
	int fromlen = sizeof(from);
	int result;
	FREObject asException;
	FREObject as3_socket;
	FREObject as3_firedata;
	FREObject as3_firedata_args[2];
	FREObject as3_byteArray;
	FREObject as3_length;
	FREObject as3_result;
	FREByteArray byteArray;

	FREGetObjectAsInt32(argv[0], &id);
	sock = &sockets[id];

	if (!sock->connected) return NULL;

	as3_socket = argv[1];
	as3_firedata = argv[2];

	while (1) {
		result = select(sock->s + 1, &sock->readfds, NULL, NULL, &select_timeout);
		if (result == SOCKET_ERROR) {
			printf("[AS3_NET_PollUDP] select: error: %d\n", WSAGetLastError());
			return NULL;
		} else if (result == 0) {
			FD_SET(sock->s, &sock->readfds);
			break;
		}
		
		buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
		result = recvfrom(sock->s, buffer, BUFFER_SIZE, 0, (sockaddr*)&from, &fromlen);

		if (result != SOCKET_ERROR) {
			if (from.sin_addr.s_addr != sock->si_server.sin_addr.s_addr) return NULL;

			FRENewObjectFromInt32(BUFFER_SIZE, &as3_length);
			FRENewObject(_AIRS("flash.utils.ByteArray"), 0, NULL, &as3_byteArray, &asException);
			FRESetObjectProperty(as3_byteArray, _AIRS("length"), as3_length, &asException);
			FREAcquireByteArray(as3_byteArray, &byteArray);
			memcpy(byteArray.bytes, buffer, BUFFER_SIZE);
			FREReleaseByteArray(as3_byteArray);

			as3_firedata_args[0] = as3_socket;
			as3_firedata_args[1] = as3_byteArray;
			FRECallObjectMethod(as3_firedata, _AIRS("call"), 2, as3_firedata_args, &as3_result, &asException);


			//buffer[result] = 0;
			//printf("[AS3_NET_PollUDP] Data: %s\n", buffer);
		} else free(buffer);
	}

	return NULL;
}

FREObject AS3_NET_SendUDP(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	FREByteArray byteArray;
	int id;
	int offset;
	int length;
	const char* address;
	uint32_t address_length;
	int port;
	uint8_t* data;
	int sent_length;

	if (FREGetObjectAsInt32(argv[0], &id) != FRE_OK) {
		printf("[AS3_NET_SendUDP] Error 0\n");
		return NULL;
	}
	if (FREAcquireByteArray(argv[1], &byteArray) != FRE_OK) {
		printf("[AS3_NET_SendUDP] Error 1\n");
		return NULL;
	}
	if (FREGetObjectAsInt32(argv[2], &offset) != FRE_OK) {
		printf("[AS3_NET_SendUDP] Error 2\n");
		return NULL;
	}
	if (FREGetObjectAsInt32(argv[3], &length) != FRE_OK) {
		printf("[AS3_NET_SendUDP] Error 3\n");
		return NULL;
	}
	if (FREGetObjectAsUTF8(argv[4], &address_length, (const uint8_t**)&address) != FRE_OK) {
		printf("[AS3_NET_SendUDP] Error 4\n");
		return NULL;
	}
	if (FREGetObjectAsInt32(argv[5], &port) != FRE_OK) {
		printf("[AS3_NET_SendUDP] Error 5\n");
		return NULL;
	}

	net_setdestinaton(sockets[id], address, port);

	// printf("[AS3_NET_SendUDP] Sending %d bytes.\n", length);
	data = byteArray.bytes;
	sendto(sockets[id].s, (char*) data, length, 0, (sockaddr*) &sockets[id].si_server, sizeof(sockets[0].si_server));

	FREReleaseByteArray(argv[1]);
	return NULL;
}

FREObject AS3_NET_DNS(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	const char *domain;
	uint32_t domain_length;
	int result;
	struct addrinfo* allDomains;
	FREObject as3_ip;
	sockaddr_in ipv4;
	char* output;

	if (FREGetObjectAsUTF8(argv[0], &domain_length, (const uint8_t**) &domain) != FRE_OK) {
		return NULL;
	}

	result = getaddrinfo(domain, NULL, NULL, &allDomains);

	if (!result) {
		ipv4 = *((sockaddr_in*)allDomains[0].ai_addr);
		output = (char*)malloc(sizeof(char) * 64);
		inet_ntop(AF_INET, &(ipv4.sin_addr), output, 64);
		FRENewObjectFromUTF8(strlen(output), (uint8_t*) output, &as3_ip);
		return as3_ip;
	} else {
		printf("[AS3_NET_DNS] getaddrinfo couldn't find domain %s. Error: %d\n", domain, result);
	}

	return NULL;
}

void net_init(FREContext ctx) {
	for (int i = 0; i < MAX_SOCKETS; i++) sockets[i].active = false;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("[WINSOCK] Initialization Failed (%d)\n", WSAGetLastError());
		return;
	}
	select_timeout.tv_sec = 0;
	select_timeout.tv_usec = 10;
	printf("[WINSOCK] Initialized\n");
	initialized = true;
}

void net_close() {
	if (!initialized) return;

	for (int i = 0; i < MAX_SOCKETS; i++) {
		if (!sockets[i].active) continue;
		net_freesocket(sockets[i]);
	}

	WSACleanup();
}