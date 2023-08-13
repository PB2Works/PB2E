#include <stdio.h>
#include <windows.h>
#include "inputex.h"
#include "main.h"

// #define TIME_INPUTS
#define MAX_BUFF_KEYS 100

HWND input_window;

#ifdef TIME_INPUTS
__int64 inputThreadStartTime;
#endif

static BOOL kb_type[MAX_BUFF_KEYS];
static DWORD kb_keys[MAX_BUFF_KEYS];
static char kb_keyChars[MAX_BUFF_KEYS];
static int kb_inQueue;
static LONG lastMouseX, lastMouseY;
static BOOL sent_mouse_up;

static HANDLE hThreadInput = NULL;
static DWORD dwThreadInput = 0;

#ifdef TIME_INPUTS
__int64 debug_hptFreq;
#endif

DWORD WINAPI THREAD_input(void* vargp);
void inputex_init(FREContext ctx) {
	FREObject asKbBuff, asInt;
	FRENewObjectFromInt32(MAX_BUFF_KEYS, &asInt);
	FRENewObject(_AIRS("Vector.<flash.events.KeyboardEvent>"), 1, &asInt, &asKbBuff, &asException);
	FRENewObject(_AIRS("Object"), 0, NULL, &actionScriptData, &asException);
	FRESetObjectProperty(actionScriptData, _AIRS("k_ev"), asKbBuff, &asException);
	FRESetContextActionScriptData(ctx, actionScriptData);
	kb_inQueue = 0;
	hThreadInput = CreateThread(NULL, 0, THREAD_input, NULL, 0, &dwThreadInput);
}

void inputex_close() {
	if (hThreadInput != NULL) TerminateThread(hThreadInput, 0);
}

/*
static LRESULT CALLBACK NewWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	if (hWnd == main_window) {
		switch (message)
		{
		case WM_CHAR:
		case WM_KEYUP:
		case WM_KEYDOWN:
			// CallWindowProc(main_wproc, hWnd, message, wParam, lParam);
			//printf("Key pressed! %d\n", wParam);
			break;
		case WM_DESTROY:
			SetWindowLong(main_window, GWL_WNDPROC, (LONG)main_wproc);
			break;
		}
	}
	CallWindowProc(main_wproc, hWnd, message, wParam, lParam);
	return TRUE;
}
*/

static int int_inpp = 0;
static LRESULT CALLBACK inputWindowProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	BYTE keyboardState[256];
	WORD outChars;

	switch (Message) {
		case WM_INPUT:
		{
			UINT dwSize = 0;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
			LPBYTE lpb = (LPBYTE)malloc(dwSize);
			if (lpb == NULL) return 0;

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
				printf("GetRawInputData does not return correct size !\n");
			}

			RAWINPUT* raw = (RAWINPUT*)lpb;

			if (GetForegroundWindow() == main_window && raw->header.dwType == RIM_TYPEKEYBOARD)
			{
#ifdef TIME_INPUTS
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				printf("KB_RAW: %d [%ld]\n", int_inpp++, (li.QuadPart - inputThreadStartTime) * 10000LL / debug_hptFreq);
#else
				RAWKEYBOARD kb = raw->data.keyboard;
				int n;
				switch (kb.Message) {
				case WM_KEYDOWN:
					if (kb_inQueue >= MAX_BUFF_KEYS) n = MAX_BUFF_KEYS - 1;
					else n = kb_inQueue;
					kb_keys[n] = kb.VKey;
					(void)GetKeyboardState(keyboardState);
					if (!ToAscii(kb.VKey, kb.MakeCode, keyboardState, &outChars, 0)) outChars = 0;
					kb_keyChars[n] = (char)outChars;
					kb_type[n] = 0;
					// printf("KB_RAW_DOWN %d: %d, \"%c\"\n", int_inpp++, kb.VKey, (char)outChars);
					FREDispatchStatusEventAsync(fre_ctx, _AIRS("key"), _AIRS(""));
					if (++kb_inQueue >= MAX_BUFF_KEYS) kb_inQueue = MAX_BUFF_KEYS;
					break;
				case WM_KEYUP:
					if (kb_inQueue >= MAX_BUFF_KEYS) n = MAX_BUFF_KEYS - 1;
					else n = kb_inQueue;
					kb_keys[n] = kb.VKey;
					(void)GetKeyboardState(keyboardState);
					if (!ToAscii(kb.VKey, kb.MakeCode, keyboardState, &outChars, 0)) outChars = 0;
					kb_keyChars[n] = (char)outChars;
					kb_type[n] = 1;
					// printf("KB_RAW_UP %d: %d, \"%c\"\n", int_inpp++, kb.VKey, (char)outChars);
					FREDispatchStatusEventAsync(fre_ctx, _AIRS("key"), _AIRS(""));
					if (++kb_inQueue >= MAX_BUFF_KEYS) kb_inQueue = MAX_BUFF_KEYS;
					break;
				}
#endif
			}
			else if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				POINT mousePos;
				RAWMOUSE ms = raw->data.mouse;
				BOOL mouseInWindow;
				GetCursorPos(&mousePos);
				mouseInWindow = !sent_mouse_up || (WindowFromPoint(mousePos) == main_window);
				if (mouseInWindow) {
					lastMouseX = mousePos.x;
					lastMouseY = mousePos.y;
				}
				if (ms.usButtonFlags != 0) {
					if (!mouseInWindow && !sent_mouse_up) {
						FREDispatchStatusEventAsync(fre_ctx, _AIRS("ms"), _AIRS("u"));
						sent_mouse_up = true;
					}
					if (mouseInWindow) {
						// printf("Mouse state: %d\n", ms.usButtonFlags);
						if (ms.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN) {
							FREDispatchStatusEventAsync(fre_ctx, _AIRS("ms"), _AIRS("d"));
							sent_mouse_up = false;
						}
						else if (ms.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP) {
							FREDispatchStatusEventAsync(fre_ctx, _AIRS("ms"), _AIRS("u"));
							sent_mouse_up = true;
						}
					}
				}
				// printf("RawMouse: %d %d\n", ms.lLastX, ms.lLastY);
			}

			free(lpb);
			return 0;
		}
	}
	return DefWindowProc(hwnd, Message, wParam, lParam);
}

ANEFunction(pollMouse) {
	POINT mPoint;
	RECT windowSize;
	FREObject x, y;
	mPoint.x = lastMouseX;
	mPoint.y = lastMouseY;
	ScreenToClient(main_window, &mPoint);
	GetClientRect(main_window, &windowSize);
	FRENewObjectFromDouble(((double)mPoint.x) * stageWidth / ((double)windowSize.right), &x);
	FRENewObjectFromDouble(((double)mPoint.y) * stageHeight / ((double)windowSize.bottom), &y);
	FREGetContextActionScriptData(ctx, &actionScriptData);
	FRESetObjectProperty(actionScriptData, _AIRS("mx"), x, &asException);
	FRESetObjectProperty(actionScriptData, _AIRS("my"), y, &asException);
	return NULL;
}

// keyDown, keyUp
ANEFunction(pollKeys) {
	FREObject ex_n;
	FREObject kev, kevs;
	FREObject args[5];
	FREGetContextActionScriptData(ctx, &actionScriptData);
	FREGetObjectProperty(actionScriptData, _AIRS("k_ev"), &kevs, &asException);
	for (int i = 0; i < kb_inQueue; i++) {
		if (kb_type[i]) FRENewObjectFromUTF8(6, _AIRS("keyUp"), &args[0]);
		else            FRENewObjectFromUTF8(8, _AIRS("keyDown"), &args[0]);
		FRENewObjectFromBool(true, &args[1]);
		FRENewObjectFromBool(false, &args[2]);
		FRENewObjectFromUint32((uint32_t)(kb_keyChars[i]), &args[3]);
		FRENewObjectFromUint32(kb_keys[i], &args[4]);
		FRENewObject(_AIRS("flash.events.KeyboardEvent"), 5, args, &kev, &asException);
		FRESetArrayElementAt(kevs, i, kev);
	}
	FRENewObjectFromUint32(kb_inQueue, &ex_n);
	FRESetObjectProperty(actionScriptData, _AIRS("k_n"), ex_n, &asException);
	kb_inQueue = 0;
	return NULL;
}

DWORD WINAPI THREAD_input(void* vargp) {
	RAWINPUTDEVICE Rid[2];
	MSG msg;

#ifdef TIME_INPUTS
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	inputThreadStartTime = li.QuadPart;
	QueryPerformanceFrequency(&li);
	debug_hptFreq = li.QuadPart;
#endif

	printf("Input thread created.\n");
	input_window = CreateWindowA("STATIC", "PB2Inputs", WS_VISIBLE, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	SetWindowLongA(input_window, GWL_WNDPROC, (long)inputWindowProc);
	Rid[0].usUsagePage = 0x01;           // HID_USAGE_PAGE_GENERIC
	Rid[0].usUsage = 0x02;               // HID_USAGE_GENERIC_MOUSE
	Rid[0].dwFlags = RIDEV_INPUTSINK;    // Always receive inputs
	Rid[0].hwndTarget = input_window;
	Rid[1].usUsagePage = 0x01;           // HID_USAGE_PAGE_GENERIC
	Rid[1].usUsage = 0x06;               // HID_USAGE_GENERIC_KEYBOARD
	Rid[1].dwFlags = RIDEV_INPUTSINK;    // Always receive inputs
	Rid[1].hwndTarget = input_window;
	sent_mouse_up = true;
	if (RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE)
	{
		printf("Error: Raw Input registration failed.\n");
	}
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		DispatchMessage(&msg);
	}
	return 0;
}