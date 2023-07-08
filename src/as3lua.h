#pragma once
#include "ane.h"

void lua_init(FREContext ctx);
void lua_close();
FREObject AS3_LUA(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_Close(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_DoString(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_GetError(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_SetGlobal(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_GetGlobal(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_CallFunction(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_RegisterArgument(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_CallRegisteredFunction(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_RegisterGlobalMetatable(FREContext, void*, uint32_t, FREObject[]);
FREObject AS3_LUA_NewMetaObject(FREContext, void*, uint32_t, FREObject[]);