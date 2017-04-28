#include <sl.h>
#include "Hardware\GPU\adl.h"

#define WS_EX_TOPMOST 0x00000008L
#define WM_USER                         0x0400
#define TBM_SETPOS              (WM_USER+5)
#define TBM_SETRANGE            (WM_USER+6)
#define TBM_SETSEL              (WM_USER+10)
#define TBM_SETPAGESIZE         (WM_USER+21)
#define WM_HSCROLL                      0x0114
#define TBM_GETPOS              (WM_USER)

HANDLE Engine;


HANDLE CreateTrackbarControl(
	HANDLE Parent,
	INT32 Y,
	HANDLE* text
	)
{
	HANDLE windowHandle;
	HANDLE textHandle;

	textHandle = CreateWindowExW(
		0,
		L"static",
		L"Pstate",
		WS_CHILD | WS_VISIBLE,
		20,
		Y,
		40,
		20,
		Parent,
		0,
		0,
		0
		);

	if (text)
		*text = textHandle;
	
	windowHandle = CreateWindowExW(
		0,
		L"msctls_trackbar32",
		NULL,
		WS_CHILD | WS_VISIBLE,
		20,
		Y + 20,
		305,
		37,
		Parent,
		0,
		0,
		0
		);

	SendMessageW(windowHandle, TBM_SETRANGE,
		TRUE,                   // redraw flag 
		MAKELONG(0, 900));  // min. & max. positions

	SendMessageW(windowHandle, TBM_SETPAGESIZE,
		0, 4);                  // new page size 

	SendMessageW(windowHandle, TBM_SETSEL,
		FALSE,                  // redraw flag 
		MAKELONG(0, 900));

	SendMessageW(windowHandle, TBM_SETPOS,
		TRUE,                   // redraw flag 
		500);

	return windowHandle;
}


HANDLE textHandle;
HANDLE EngineHandle;
HANDLE MemoryHandle;
HANDLE VoltageHandle;
HANDLE FanHandle;
HANDLE combo;
#define COMBO 100
#define APPLY 101

INT32 __stdcall OverlockWndProc(
	VOID *hWnd,
	UINT32 uMessage,
	UINT32 wParam,
	LONG lParam
	)
{
	int trackBarY = 60;
	DWORD dwPos;
	int engine;
	int memory;
	int voltage;
	int fan;

	int pstate;

	switch (uMessage)
	{
	case WM_CREATE:
		CreateWindowExW(
			0,
			L"static",
			L"Pstate",
			WS_CHILD | WS_VISIBLE,
			20,
			20,
			40,
			20,
			hWnd,
			0,
			0,
			0
			);

		combo = CreateWindowExW(
			0,
			L"ComboBox",
			L"",
			CBS_DROPDOWN | WS_CHILD | WS_VISIBLE,
			80,
			20,
			100,
			20,
			hWnd,
			(void*) COMBO,
			0,
			0
			);

		SendMessageW(combo, CB_ADDSTRING, 0, (LONG)L"1");
		SendMessageW(combo, CB_ADDSTRING, 0, (LONG)L"2");
		SendMessageW(combo, CB_ADDSTRING, 0, (LONG)L"3");
		
		EngineHandle = CreateTrackbarControl(hWnd, trackBarY, &textHandle);
		MemoryHandle = CreateTrackbarControl(hWnd, trackBarY+=70, NULL);
		VoltageHandle = CreateTrackbarControl(hWnd, trackBarY+=70, NULL);
		FanHandle = CreateTrackbarControl(hWnd, trackBarY += 70, NULL);

		CreateWindowExW(
			0,
			L"button",
			L"Apply",
			WS_CHILD | WS_VISIBLE,
			150,
			320,
			75,
			25,
			hWnd,
			(void*) APPLY,
			0,
			0
			);

		break;

	case WM_HSCROLL:
		dwPos = SendMessageW((void*)lParam, TBM_GETPOS, 0, 0);
		wchar_t lol[260];

		String_Format(lol, 260, L"%i", dwPos);
		SetWindowTextW(textHandle, lol);
		break;
	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case COMBO:
			break;
		case APPLY:
			pstate = SendMessageW(combo, CB_GETCURSEL, 0, 0);

			engine = SendMessageW(EngineHandle, TBM_GETPOS, 0, 0);
			memory = SendMessageW(MemoryHandle, TBM_GETPOS, 0, 0);
			voltage = SendMessageW(VoltageHandle, TBM_GETPOS, 0, 0);
			fan = SendMessageW(FanHandle, TBM_GETPOS, 0, 0);

			Adl_SetEngineClock(engine, pstate);
			Adl_SetMemoryClock(memory, pstate);
			//Adl_SetVoltage(voltage);
			//Adl_SetFan
			break;
		}
		break;
	}

	return DefWindowProcW(
		hWnd,
		uMessage,
		wParam,
		lParam
		);
}


VOID Overclock()
{
	HANDLE windowHandle;
	
	WNDCLASSEX windowClass = { 0 };

	windowClass.WndProc = OverlockWndProc;
	windowClass.ClassName = L"overclock";
	windowClass.Size = sizeof(WNDCLASSEX);
	windowClass.Icon = NULL;
	windowClass.IconSm = NULL;

	RegisterClassExW(&windowClass);
	windowHandle = CreateWindowExW(
		0,
		L"overclock",
		L"Overclocking",
		WS_SYSMENU | WS_VISIBLE,
		((int)0x80000000), //CW_USEDEFAULT
		NULL,
		376,
		400,
		0,
		0,
		0,
		0
		);

	ShowWindow(windowHandle, 1);
}