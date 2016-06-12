#include <Windows.h>
#include "overlay.h"
#include <OvRender.h>
#include <detourxs.h>

#include "menu.h"
#include "kbhook.h"


typedef struct _KEYBOARD_HOOK_PARAMS
{
    __int32 HookType;
    __int32 ProcessId;
    HOOKPROC HookProcedure;

}KEYBOARD_HOOK_PARAMS;


HANDLE ProcessHeap;
HHOOK KeyboardHookHandle;


VOID PushKeySwapCallback( LPMSG Message )
{
    switch (Message->wParam)
    {
    case 'W':
    Message->wParam = VK_UP;
    break;
    case 'A':
    Message->wParam = VK_LEFT;
    break;
    case 'S':
    Message->wParam = VK_DOWN;
    break;
    case 'D':
    Message->wParam = VK_RIGHT;
    break;
    }
}


BOOLEAN MessageHook( LPMSG Message )
{
    static BOOLEAN ignoreRawInput = FALSE;
    static BOOLEAN usingRawInput = FALSE;

    switch (Message->message)
    {
    case WM_KEYDOWN:
    {
        ignoreRawInput = TRUE;

        if (usingRawInput)
        {
            usingRawInput = FALSE;
            return TRUE;
        }

        MenuKeyboardHook(Message->wParam);

        if (PushSharedMemory->SwapWASD)
        {
            PushKeySwapCallback(Message);
        }

        if (PushSharedMemory->DisableRepeatKeys)
        {
            int repeatCount = (Message->lParam & 0x40000000);

            if (repeatCount)
                return FALSE;
        }

    } break;

    case WM_KEYUP:
    {
        if (PushSharedMemory->SwapWASD)
        {
            PushKeySwapCallback(Message);
        }

    } break;

    case WM_CHAR:
    {
        if (PushSharedMemory->DisableRepeatKeys)
        {
            int repeatCount = (Message->lParam & 0x40000000);

            if (repeatCount)
                return FALSE;
        }

    } break;

    case WM_INPUT:
    {
        UINT dwSize;
        RAWINPUT *buffer;

        if (ignoreRawInput)
            return TRUE;

        // Request size of the raw input buffer to dwSize
        GetRawInputData(
            (HRAWINPUT)Message->lParam,
            RID_INPUT,
            NULL,
            &dwSize,
            sizeof(RAWINPUTHEADER)
            );

        // allocate buffer for input data
        buffer = (RAWINPUT*)HeapAlloc(ProcessHeap, 0, dwSize);

        if (GetRawInputData(
            (HRAWINPUT)Message->lParam,
            RID_INPUT,
            buffer,
            &dwSize,
            sizeof(RAWINPUTHEADER)))
        {
            // if this is keyboard message and WM_KEYDOWN, process
            // the key
            if (buffer->header.dwType == RIM_TYPEKEYBOARD
                && buffer->data.keyboard.Message == WM_KEYDOWN)
            {
                usingRawInput = TRUE;

                MenuKeyboardHook(buffer->data.keyboard.VKey);
            }
        }

        // free the buffer
        HeapFree(ProcessHeap, 0, buffer);
    } break;
    }

    return TRUE;
}


LRESULT CALLBACK MessageProc(
    __int32 nCode, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
	wchar_t fileName[260];

    if (nCode == HC_ACTION && wParam & PM_REMOVE)
    {
        MessageHook((LPMSG)lParam);
    }

	GetModuleFileNameW(NULL, fileName, 260);
	
	if (wcscmp(fileName, L"E:\\Steam\\steamapps\\common\\Tom Clany's HAWX\\HAWX.exe") == 0)
	{
		wchar_t output[260];
		MSG *msg;

		//swprintf(output, L"MessageProc(%i, 0x%x, 0x%x)", nCode, wParam, lParam);
		msg = (MSG*) lParam;

		swprintf(
			output,
			L"MessageProc(0x%x, %i, 0x%x, 0x%x, %i, %i, %i)",
			msg->hwnd, 
			msg->message, 
			msg->wParam,
			msg->lParam,
			msg->time,
			msg->pt.x,
			msg->pt.y
			);

		OutputDebugStringW(output);
	}

    return CallNextHookEx(KeyboardHookHandle, nCode, wParam, lParam);
}


LRESULT CALLBACK KeyboardProc(
    __int32 nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if ((0x80000000 & lParam) == 0)//key down
    {
        MenuKeyboardHook(wParam);
    }

    return CallNextHookEx(KeyboardHookHandle, nCode, wParam, lParam);
}


BOOL CALLBACK MessageHookWindowEnum(HWND hwnd, LPARAM lParam)
{
    DWORD processId;
    DWORD threadId;

    threadId = GetWindowThreadProcessId(hwnd, &processId);

    if (processId == ((KEYBOARD_HOOK_PARAMS*)lParam)->ProcessId)
    {
        if (!KeyboardHookHandle)
        {
            KeyboardHookHandle = SetWindowsHookEx(
                ((KEYBOARD_HOOK_PARAMS*)lParam)->HookType,
                ((KEYBOARD_HOOK_PARAMS*)lParam)->HookProcedure,
                GetModuleHandleW(NULL),
                threadId
                );

            if (KeyboardHookHandle)
                OutputDebugStringW(L"Message hook success!");
            else
                OutputDebugStringW(L"Message hook failure!");
        }
    }

    return TRUE;
}


typedef BOOL(WINAPI* TYPE_PeekMessageW)(
    LPMSG lpMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg
    );

typedef BOOL(WINAPI* TYPE_PeekMessageA)(
    LPMSG lpMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg
    );

VOID* DetourApi(
    WCHAR* dllName, 
    CHAR* apiName, 
    BYTE* NewFunction
    );


TYPE_PeekMessageW       PushPeekMessageW;
TYPE_PeekMessageA       PushPeekMessageA;


BOOL WINAPI PeekMessageWHook(
    _In_ LPMSG lpMsg,
    _In_ HWND hWnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax,
    _In_ UINT wRemoveMsg
    )
{
    BOOL result;

    result = PushPeekMessageW(
        lpMsg,
        hWnd,
        wMsgFilterMin,
        wMsgFilterMax,
        wRemoveMsg
        );

    if (result && wRemoveMsg & PM_REMOVE)
    {
        result = MessageHook(lpMsg);
    }

    return result;
}


BOOL WINAPI PeekMessageAHook(
    _In_ LPMSG lpMsg,
    _In_ HWND hWnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax,
    _In_ UINT wRemoveMsg
    )
{
    BOOL result;

    result = PushPeekMessageA(
        lpMsg,
        hWnd,
        wMsgFilterMin,
        wMsgFilterMax,
        wRemoveMsg
        );

    if (result && wRemoveMsg & PM_REMOVE)
    {
        result = MessageHook(lpMsg);
    }

    return result;
}


void InitializeDetourHook()
{
    PushPeekMessageW = (TYPE_PeekMessageW)DetourApi(
        L"user32.dll",
        "PeekMessageW",
        (BYTE*)PeekMessageWHook
        );

    PushPeekMessageA = (TYPE_PeekMessageA)DetourApi(
        L"user32.dll",
        "PeekMessageA",
        (BYTE*)PeekMessageAHook
        );
}


void Keyboard_Hook( KEYBOARD_HOOK_TYPE HookType )
{
    KEYBOARD_HOOK_PARAMS keyboardHook;

    ProcessHeap = GetProcessHeap();

    keyboardHook.ProcessId = GetCurrentProcessId();

    switch (HookType)
    {
    case KEYBOARD_HOOK_MESSAGE:
        keyboardHook.HookProcedure = MessageProc;
        keyboardHook.HookType = WH_GETMESSAGE;
        EnumWindows(MessageHookWindowEnum, (LPARAM)&keyboardHook);
        break;
    case KEYBOARD_HOOK_KEYBOARD:
        keyboardHook.HookProcedure = KeyboardProc;
        keyboardHook.HookType = WH_KEYBOARD;
        EnumWindows(MessageHookWindowEnum, (LPARAM)&keyboardHook);
        break;
    case KEYBOARD_HOOK_DETOURS:
        InitializeDetourHook();
        KeyboardHookHandle = (HHOOK) 0xffffffff; //LOL
        break;
    default:
        keyboardHook.HookProcedure = NULL;
        break;
    }
}