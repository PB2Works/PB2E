#pragma once
#include <FlashRuntimeExtensions.h>
#define _AIRS(s) ((const uint8_t*)(s))

#define AIR_CHECKERRORS

#ifdef AIR_CHECKERRORS
#define _AIRCHECK(f, e) if ((f) != FRE_OK) { \
	printf("<CHECK> %s\n", e); \
	return NULL; \
}
#else
#define _AIRCHECK(f, e) f
#endif

#ifdef AIR_CHECKERRORS
#define _AIRINITEXCEPTIONS() FREObject asException, asException2, asExceptionMessage; \
							 uint32_t asExceptionMessageLength; \
							 const uint8_t* asExceptionString;
#else
#define _AIRINITEXCEPTIONS() 
#endif

#ifdef AIR_CHECKERRORS
#define _AIRTRY(f, e) if ((f) != FRE_OK) { \
	FREGetObjectProperty(asException, _AIRS("message"), &asExceptionMessage, &asException2); \
	FREGetObjectAsUTF8(asExceptionMessage, &asExceptionMessageLength, &asExceptionString); \
	printf("%s: <EXCEPTION> %s\n", e, asExceptionString); \
	return NULL; \
}
#else
#define _AIRTRY(f, e) f
#endif

#define ANE_CURRENT_FUNCTION "None"
#define ANEFunction(name) FREObject AS3_##name(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])