#pragma once
#include <malloc.h>
#include <sapi.h>
#include <timeapi.h>
#include <stdio.h>
#include "main.h"
#include "inputex.h"
#include "as3lua.h"
#include "net.h"

HWND main_window;
FREContext fre_ctx;
FREObject actionScriptData;
FREObject asException;
double stageWidth;
double stageHeight;

// WNDPROC main_wproc;

void contextFinalizer(FREContext ctx) {}
static void emain();

extern "C"
{
	__declspec(dllexport) void extensionInitializer(void** dataToSet, FREContextInitializer* contextInitializer, FREContextFinalizer* contextFinalizer);
	__declspec(dllexport) void extensionFinalizer(void* extData);
}

FREObject AS3_setStageSize(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	if (argc >= 2) {
		FREGetObjectAsDouble(argv[0], &stageWidth);
		FREGetObjectAsDouble(argv[1], &stageHeight);
		printf("[ANE] setStageSize: %lfx%lf\n", stageWidth, stageHeight);
	}
	return NULL;
}

#define N_PERF_COUNTERS 20
static LARGE_INTEGER perfCounters[N_PERF_COUNTERS];

FREObject AS3_getPerformanceFrequency(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	LARGE_INTEGER li;
	FREObject as3_n;
	QueryPerformanceFrequency(&li);
	FRENewObjectFromUint32(li.QuadPart, &as3_n);
	return as3_n;
}

FREObject AS3_startMeasure(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	uint32_t n;
	FREGetObjectAsUint32(argv[0], &n);
	QueryPerformanceCounter(&(perfCounters[n]));
	return NULL;
}

FREObject AS3_stopMeasure(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]) {
	uint32_t n;
	LARGE_INTEGER li;
	FREObject as3_value;
	FREGetObjectAsUint32(argv[0], &n);
	QueryPerformanceCounter(&li);
	FRENewObjectFromUint32(li.QuadPart - perfCounters[n].QuadPart, &as3_value);
	return as3_value;
}

void contextInitializer(void* extData, const uint8_t* ctxType, FREContext ctx, uint32_t* numFunctionsToSet, const FRENamedFunction** functionsToSet)
{
#define A3F(id, fname, func) funcs[id].name = _AIRS(fname); \
							 funcs[id].functionData = NULL; \
							 funcs[id].function = &AS3_##func
#define NUM_AS3_FUNCS 22
	FRENamedFunction* funcs = (FRENamedFunction*) malloc(sizeof(FRENamedFunction) * NUM_AS3_FUNCS);
	if (funcs != NULL) {
		A3F(0, "pollMouse", pollMouse);
		A3F(1, "pollKeys", pollKeys);
		A3F(2, "setStageSize", setStageSize);
		A3F(3, "LUA", LUA);
		A3F(4, "LUA_Close", LUA_Close);
		A3F(5, "LUA_DoString", LUA_DoString);
		A3F(6, "LUA_GetError", LUA_GetError);
		A3F(7, "LUA_SetGlobal", LUA_SetGlobal);
		A3F(8, "LUA_GetGlobal", LUA_GetGlobal);
		A3F(9, "LUA_CallFunction", LUA_CallFunction);
		A3F(10, "LUA_RegArg", LUA_RegisterArgument);
		A3F(11, "LUA_CRegFunc", LUA_CallRegisteredFunction);
		A3F(12, "LUA_DefMeta", LUA_RegisterGlobalMetatable);
		A3F(13, "LUA_NewMeta", LUA_NewMetaObject);
		A3F(14, "perfFrequency", getPerformanceFrequency);
		A3F(15, "t1", startMeasure);
		A3F(16, "t2", stopMeasure);
		A3F(17, "NET_NewUDP", NET_newUDPSocket);
		A3F(18, "NET_CloseUDP", NET_closeUDPSocket);
		A3F(19, "NET_PollUDP", NET_PollUDP);
		A3F(20, "NET_SendUDP", NET_SendUDP);
		A3F(21, "NET_DNS", NET_DNS);
		*numFunctionsToSet = NUM_AS3_FUNCS;
		*functionsToSet = funcs;
	} else {
		*numFunctionsToSet = 0;
		*functionsToSet = NULL;
	}

#undef A3F

	fre_ctx = ctx;
	emain();
	// SetWindowPos(main_window, (HWND)HWND_TOP, 0, 0, 800 * 3, 400 * 3, SWP_NOREDRAW | SWP_NOREPOSITION | SWP_SHOWWINDOW);
}

#ifdef HAS_CONSOLE
static void SetupConsole() {
	AllocConsole();
	(void) freopen("CONOUT$", "w", stderr);
	(void) freopen("CONOUT$", "w", stdout);
	(void) freopen("CONIN$",  "r", stdin);
	SetConsoleTitleA("ANE Debug console");
}
#endif

static BOOL CALLBACK enumWindowCallback(HWND hwnd, LPARAM lparam) {
	char className[256];
	GetClassNameA(hwnd, className, 256);
	if (!strcmp(className, "ApolloRuntimeContentWindow")) main_window = hwnd;
	return TRUE;
}

static void emain() {
#ifdef HAS_CONSOLE
		SetupConsole();
#endif
	printf("[ANE] Begin\n");
	main_window = NULL;
	EnumThreadWindows(GetCurrentThreadId(), enumWindowCallback, NULL);
	if (main_window != NULL) {
		// main_wproc = (WNDPROC)SetWindowLongA(main_window, GWL_WNDPROC, (long)NewWindowProc);
		inputex_init(fre_ctx);
		lua_init(fre_ctx);
		net_init(fre_ctx);
#ifdef HAS_CONSOLE
		SetForegroundWindow(main_window);
#endif
	}
	else {
		printf("[ANE] ERROR: COULDN'T FIND CONTENT WINDOW.");
	}
}

void extensionInitializer(void** extData, FREContextInitializer* ctxInitializer, FREContextFinalizer* ctxFinalizer) {
	*extData = NULL; // No extension data, assume we're only running one instance
	*ctxInitializer = &contextInitializer;
	*ctxFinalizer = &contextFinalizer;
}

void extensionFinalizer(void* extData) {
	inputex_close(); // TODO: Properly close threads (send a close message & wait)
	lua_close();
	net_close();
	FreeConsole();
}