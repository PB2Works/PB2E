#pragma once
#include "ane.h"

void inputex_init(FREContext ctx);
void inputex_close();
FREObject AS3_pollMouse(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);
FREObject AS3_pollKeys(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);