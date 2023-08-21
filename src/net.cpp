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

static as3socket sockets[MAX_SOCKETS];
static bool initialized = false;
static struct timeval select_timeout;
static WSADATA wsa;

static void net_setdestinaton(as3socket& sock, const char* ip, int port) {
	sockaddr_in& si = sock.si_server;
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

ANEFunction(NET_newUDPSocket) {
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

ANEFunction(NET_closeUDPSocket) {
	int id;
	FREGetObjectAsInt32(argv[0], &id);

	if (id >= 0 && id < MAX_SOCKETS) {
		sockets[id].active = false;
		net_freesocket(sockets[id]);
	}

	return NULL;
}

ANEFunction(NET_PollUDP) {
	int id;
	as3socket* sock;
	sockaddr_in from;
	int fromlen;
	int result;
	FREObject asException;
	FREObject as3_firedata;
	FREObject as3_firedata_args[2];
	FREObject as3_byteArray;
	FREObject as3_length;
	FREObject as3_result;
	FREByteArray byteArray;

	FREGetObjectAsInt32(argv[0], &id);
	sock = &sockets[id];

	if (!sock->connected) return NULL;

	as3_firedata = argv[2];
	as3_firedata_args[0] = argv[1]; // Reference to calling NativeDatagramSocket

	fromlen = sizeof(from);
	while (1) {
		result = select(sock->s + 1, &sock->readfds, NULL, NULL, &select_timeout);
		if (result == SOCKET_ERROR) {
			printf("[AS3_NET_PollUDP] select: error: %d\n", WSAGetLastError());
			return NULL;
		}
		else if (result == 0) {
			FD_SET(sock->s, &sock->readfds);
			break;
		}

		FRENewObjectFromInt32(BUFFER_SIZE, &as3_length);
		FRENewObject(_AIRS("flash.utils.ByteArray"), 0, NULL, &as3_byteArray, &asException);
		FRESetObjectProperty(as3_byteArray, _AIRS("length"), as3_length, &asException);
		FREAcquireByteArray(as3_byteArray, &byteArray);
		result = recvfrom(sock->s, (char*)byteArray.bytes, BUFFER_SIZE, 0, (sockaddr*)&from, &fromlen);
		FREReleaseByteArray(as3_byteArray);
		if (result != SOCKET_ERROR) {
			if (from.sin_addr.s_addr != sock->si_server.sin_addr.s_addr) {
				continue;
			}
			as3_firedata_args[1] = as3_byteArray;
			FRECallObjectMethod(as3_firedata, _AIRS("call"), 2, as3_firedata_args, &as3_result, &asException);
		}
	}

	return NULL;
}

ANEFunction(NET_SendUDP) {
	FREByteArray byteArray;
	int id;
	int offset;
	int length;
	const char* address;
	uint32_t address_length;
	int port;
	uint8_t* data;
	int sent_length;

	_AIRCHECK(FREGetObjectAsInt32(argv[0], &id), "[AS3_NET_SendUDP] Error 0");
	_AIRCHECK(FREAcquireByteArray(argv[1], &byteArray), "[AS3_NET_SendUDP] Error 1");
	_AIRCHECK(FREGetObjectAsInt32(argv[2], &offset), "[AS3_NET_SendUDP] Error 2");
	_AIRCHECK(FREGetObjectAsInt32(argv[3], &length), "[AS3_NET_SendUDP] Error 3");
	_AIRCHECK(FREGetObjectAsUTF8(argv[4], &address_length, (const uint8_t**)&address), "[AS3_NET_SendUDP] Error 4");
	_AIRCHECK(FREGetObjectAsInt32(argv[5], &port), "[AS3_NET_SendUDP] Error 5");

	net_setdestinaton(sockets[id], address, port);

	// printf("[AS3_NET_SendUDP] Sending %d bytes.\n", length);
	data = byteArray.bytes;
	sendto(sockets[id].s, (char*)data, length, 0, (sockaddr*)&sockets[id].si_server, sizeof(sockets[0].si_server));

	FREReleaseByteArray(argv[1]);
	return NULL;
}

ANEFunction(NET_DNS) {
	const char* domain;
	uint32_t domain_length;
	int result;
	struct addrinfo* allDomains;
	FREObject as3_ip;
	sockaddr_in ipv4;
	char output[32];

	if (FREGetObjectAsUTF8(argv[0], &domain_length, (const uint8_t**)&domain) != FRE_OK) {
		return NULL;
	}

	result = getaddrinfo(domain, NULL, NULL, &allDomains);

	if (!result) {
		ipv4 = *((sockaddr_in*)allDomains[0].ai_addr);
		inet_ntop(AF_INET, &(ipv4.sin_addr), output, 32);
		FRENewObjectFromUTF8(strlen(output), (uint8_t*)output, &as3_ip);
		return as3_ip;
	}
	else {
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