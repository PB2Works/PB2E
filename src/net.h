#pragma once
#include "main.h"

void net_init(FREContext ctx);
void net_close();

FREObject AS3_NET_newUDPSocket(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);
FREObject AS3_NET_closeUDPSocket(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);
FREObject AS3_NET_PollUDP(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);
FREObject AS3_NET_SendUDP(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);
FREObject AS3_NET_DNS(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);