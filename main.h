#pragma once
#include <windows.h>
#include "ane.h"

extern HWND main_window;
extern FREContext fre_ctx;
extern FREObject actionScriptData;
extern FREObject asException;
extern double stageWidth;
extern double stageHeight;

// #define HAS_CONSOLE

/*
#ifndef HAS_CONSOLE
#define printf(...) 
#endif
*/