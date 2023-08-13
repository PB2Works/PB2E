#pragma once
#include <FlashRuntimeExtensions.h>
#define _AIRS(s) ((const uint8_t*)(s))

#define _AIRCHECK(f, e) if ((f) != FRE_OK) { \
printf("%s\n", e); \
return NULL; \
}

#define _AIRTRY(f, e) if ((f) != FRE_OK) { \
printf("%s\n", e); \
return NULL; \
}

#define ANEFunction(name) FREObject AS3_##name(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])