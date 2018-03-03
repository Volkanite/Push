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
BOOLEAN RawInputProcessed;
DetourXS *RawInputDetour;

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

typedef UINT(WINAPI* TYPE_GetRawInputData)(
    _In_      HRAWINPUT hRawInput,
    _In_      UINT      uiCommand,
    _Out_opt_ LPVOID    pData,
    _Inout_   PUINT     pcbSize,
    _In_      UINT      cbSizeHeader
    );

TYPE_PeekMessageW       PushPeekMessageW;
TYPE_PeekMessageA       PushPeekMessageA;
TYPE_GetRawInputData    OverlayGetRawInputData;


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
        Menu_KeyboardHook(wParam);
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
    switch (Message->message)
    {
    case WM_KEYDOWN:
    {
        if (Message->wParam == VK_INSERT && OverlayGetRawInputData)
        {           
            //Unhook raw hook since this message hook is sufficient
            Log(L"KEYBOARD_HOOK_RAW::Destroy()");
            RawInputDetour->Destroy();
            OverlayGetRawInputData = NULL;

            //Check if raw input already processed this request or we'll
            //send it twice resulting in menu appearing then quickly
            //disappearing.
            if (RawInputProcessed)
                return TRUE;
        }

        Menu_KeyboardHook(Message->wParam);

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
        Menu_KeyboardHook(wParam);
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
        if (IsWindowVisible(hwnd))
        {
            ((KEYBOARD_HOOK_PARAMS*)lParam)->ThreadId = threadId;
            ((KEYBOARD_HOOK_PARAMS*)lParam)->WindowHandle = hwnd;

            //found, prevent further processing by setting ProcessId to 0;
            ((KEYBOARD_HOOK_PARAMS*)lParam)->ProcessId = 0;
        }
    }

    return TRUE;
}


VOID* DetourApi(
    WCHAR* dllName, 
    CHAR* apiName, 
    BYTE* NewFunction
    );


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


UINT WINAPI GetRawInputDataHook(
    _In_      HRAWINPUT hRawInput,
    _In_      UINT      uiCommand,
    _Out_opt_ LPVOID    pData,
    _Inout_   PUINT     pcbSize,
    _In_      UINT      cbSizeHeader
    )
{
    UINT result;

    result = OverlayGetRawInputData(
        hRawInput,
        uiCommand,
        pData,
        pcbSize,
        cbSizeHeader
        );

    if (pData)
    {
        RAWINPUT *data;

        data = (RAWINPUT*) pData;

        // if this is keyboard message and WM_KEYDOWN, process
        // the key
        if (data->header.dwType == RIM_TYPEKEYBOARD
            && data->data.keyboard.Message == WM_KEYDOWN)
        {
            if (data->data.keyboard.VKey == VK_INSERT)
            {
                RawInputProcessed = TRUE;
            }

            Menu_KeyboardHook(data->data.keyboard.VKey);
        }

        if (Menu_IsVisible() && data->header.dwType == RIM_TYPEKEYBOARD)
        {
            // send an invalid type. we do this for games that do not check the return
            // value of GetRawInputData(pData) and just assumes the function succeeded. 
            // Thus returning -1 is not enough, so we spoof the device type to something 
            // invalid as the game has to check what type of device generated the message
            // at some point.
            data->header.dwType = 3; //3 is not a valid type. 

            return -1;
        }
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
                Log(L"Message hook success on thread %i", keyboardHook.ThreadId);
            }
            else
            {
                Log(L"Message hook failure on thread %i", keyboardHook.ThreadId);
            }
        }

        break;

    case KEYBOARD_HOOK_DETOURS:
        InitializeDetourHook();
        KeyboardHookHandle = (HHOOK) 0xffffffff; //LOL
        break;

    case KEYBOARD_HOOK_RAW:
        if (!OverlayGetRawInputData)
        {
#ifdef _M_IX86
            BYTE *functionStart = NULL;
            HMODULE moduleHandle;
            DWORD address = 0;

            Log(L"KEYBOARD_HOOK_RAW::Create()");

            // Get the API address
            moduleHandle = GetModuleHandleW(L"user32.dll");
            address = (DWORD)GetProcAddress(moduleHandle, "GetRawInputData");

            functionStart = (BYTE*)address;
            RawInputDetour = new DetourXS(functionStart, (BYTE*)GetRawInputDataHook);

            OverlayGetRawInputData = (TYPE_GetRawInputData)RawInputDetour->GetTrampoline();
#endif 
        }
        break;
    default:
        break;
    }
}