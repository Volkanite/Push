#include <Windows.h>
#include "overlay.h"
#include <OvRender.h>

#include "menu.h"


typedef struct _KEYBOARD_HOOK_PARAMS
{
    __int32 HookType;
    __int32 ProcessId;
    HOOKPROC HookProcedure;

}KEYBOARD_HOOK_PARAMS;


HANDLE ProcessHeap;
HHOOK hMessageHook;


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
    if (nCode == HC_ACTION && wParam & PM_REMOVE)
    {
        MessageHook((LPMSG)lParam);
    }

    return CallNextHookEx(hMessageHook, nCode, wParam, lParam);
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

    return CallNextHookEx(hMessageHook, nCode, wParam, lParam);
}


BOOL CALLBACK MessageHookWindowEnum(HWND hwnd, LPARAM lParam)
{
    DWORD processId;
    DWORD threadId;

    threadId = GetWindowThreadProcessId(hwnd, &processId);

    if (processId == ((KEYBOARD_HOOK_PARAMS*)lParam)->ProcessId)
    {
        if (!hMessageHook)
        {
            hMessageHook = SetWindowsHookEx(
                ((KEYBOARD_HOOK_PARAMS*)lParam)->HookType,
                ((KEYBOARD_HOOK_PARAMS*)lParam)->HookProcedure,
                GetModuleHandleW(NULL),
                threadId
                );

            if (hMessageHook)
                OutputDebugStringW(L"Message hook success!");
            else
                OutputDebugStringW(L"Message hook failure!");
        }
    }

    return TRUE;
}


void Keyboard_Hook( __int32 HookType )
{
    KEYBOARD_HOOK_PARAMS keyboardHook;

    ProcessHeap = GetProcessHeap();

    keyboardHook.HookType = HookType;
    keyboardHook.ProcessId = GetCurrentProcessId();

    switch (HookType)
    {
    case WH_GETMESSAGE:
        keyboardHook.HookProcedure = MessageProc;
        break;
    case WH_KEYBOARD:
        keyboardHook.HookProcedure = KeyboardProc;
        break;
    default:
        keyboardHook.HookProcedure = NULL;
        break;
    }

    EnumWindows(MessageHookWindowEnum, (LPARAM)&keyboardHook);
}