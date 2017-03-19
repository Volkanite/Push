#include <Windows.h>
#include "overlay.h"
#include <OvRender.h>
#include <detourxs.h>

#include "menu.h"
#include "kbhook.h"


typedef struct _KEYBOARD_HOOK_PARAMS
{
    unsigned __int32 ProcessId;
    unsigned __int32 ThreadId;  //Out
    HWND WindowHandle;          //Out
}KEYBOARD_HOOK_PARAMS;


HANDLE ProcessHeap;
HHOOK KeyboardHookHandle;
WNDPROC OldWNDPROC;


LONG WINAPI KeyboardHook(
    HWND Handle, 
    UINT Message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    switch (Message)
    {
    case WM_KEYDOWN:
        break;

    case WM_CHAR:
        break;

    case WM_NCACTIVATE:
    case WM_ACTIVATE:
    case WM_KILLFOCUS:
        return 0;
        break;
    }

    return CallWindowProc(OldWNDPROC, Handle, Message, wParam, lParam);
}


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
    
    /*if (wcsstr(
        fileName, 
        L"E:\\Steam\\steamapps\\common\\Tom Clany's HAWX\\HAWX") == 0)
    {*/
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
    //}

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


BOOL CALLBACK MessageHookWindowEnum( HWND hwnd, LPARAM lParam )
{
    DWORD processId;
    DWORD threadId;

    threadId = GetWindowThreadProcessId(hwnd, &processId);

    if (processId == ((KEYBOARD_HOOK_PARAMS*)lParam)->ProcessId)
    {
        ((KEYBOARD_HOOK_PARAMS*)lParam)->ThreadId = threadId;
        ((KEYBOARD_HOOK_PARAMS*)lParam)->WindowHandle = hwnd;

        //found, prevent further processing by setting ProcessId to 0;
        ((KEYBOARD_HOOK_PARAMS*)lParam)->ProcessId = 0;
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


void Keyboard_Hook( PUSH_KEYBOARD_HOOK_TYPE HookType )
{
    KEYBOARD_HOOK_PARAMS keyboardHook;

    ProcessHeap = GetProcessHeap();

    keyboardHook.ProcessId = GetCurrentProcessId();

    switch (HookType)
    {
    case KEYBOARD_HOOK_SUBCLASS:
         
        if (!OldWNDPROC)
        {
            EnumWindows(MessageHookWindowEnum, (LPARAM)&keyboardHook);

            OldWNDPROC = (WNDPROC)SetWindowLongPtrW(
                keyboardHook.WindowHandle,
                GWLP_WNDPROC,
                (LONG)KeyboardHook
                );
        }
        
        break;
    case KEYBOARD_HOOK_MESSAGE:
    case KEYBOARD_HOOK_KEYBOARD:

        if (!KeyboardHookHandle)
        {
            wchar_t output[260];
            int hookId;
            HOOKPROC hookProcedure;

            EnumWindows(MessageHookWindowEnum, (LPARAM)&keyboardHook);

            if (HookType == KEYBOARD_HOOK_MESSAGE)
            {
                hookId = WH_GETMESSAGE;
                hookProcedure = MessageProc;
            }
            else if (HookType == KEYBOARD_HOOK_KEYBOARD)
            {
                hookId = WH_KEYBOARD;
                hookProcedure = KeyboardProc;
            }

            KeyboardHookHandle = SetWindowsHookExW(
                hookId,
                hookProcedure,
                GetModuleHandleW(NULL),
                keyboardHook.ThreadId
                );

            
            if (KeyboardHookHandle)
            {
                swprintf(
                    output,
                    L"Message hook success on thread %i",
                    keyboardHook.ThreadId
                    );
            }
            else
            {
                swprintf(
                    output,
                    L"Message hook failure on thread %i",
                    keyboardHook.ThreadId
                    );
            }
                
            OutputDebugStringW(output);
        }

        break;

    case KEYBOARD_HOOK_DETOURS:
        InitializeDetourHook();
        KeyboardHookHandle = (HHOOK) 0xffffffff; //LOL
        break;
    default:
        break;
    }
}