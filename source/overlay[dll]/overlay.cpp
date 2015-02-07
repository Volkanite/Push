#include <Windows.h>
#include <stdio.h>

#include "overlay.h"
#include <OvRender.h>
#include "render.h"
#include "thread.h"
#include "menu.h"
#include <sldetours.h>


CHAR *pszModuleName;
PUSH_SHARED_MEMORY   *PushSharedMemory;
void            *hEvent = NULL;
BOOLEAN dxgiHooked              = FALSE;
BOOLEAN g_DXGI                  = FALSE;
ThreadMonitor* PushThreadMonitor;
HANDLE PushProcessHeap;


VOID
PushRefreshThreadMonitor()
{
    if (PushThreadMonitor == NULL)
        PushThreadMonitor = new ThreadMonitor();

    PushThreadMonitor->Refresh();
}


UINT8
PushGetMaxThreadUsage()
{
    return PushThreadMonitor->GetMaxThreadUsage();
}


VOID
PushOptimizeThreads()
{
    PushThreadMonitor->OptimizeThreads();
}


ULONG
__stdcall
MonitorThread(LPVOID v)
{
    while (TRUE)
    {
        WaitForSingleObject(hEvent, INFINITE);

        OvCreateOverlay( RnRender );
    }

    return NULL;
}


typedef BOOL (WINAPI* TYPE_PeekMessageW)(
    LPMSG lpMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg
    );

typedef BOOL (WINAPI* TYPE_PeekMessageA)(
    LPMSG lpMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg
    );


TYPE_PeekMessageW       PushPeekMessageW;
TYPE_PeekMessageA       PushPeekMessageA;


VOID
PushKeySwapCallback( LPMSG Message )
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


BOOLEAN
MessageHook( LPMSG Message )
{
    switch (Message->message)    
    {   
        case WM_KEYDOWN:
            {
                MenuKeyboardHook(Message->wParam);
                
                if (PushSharedMemory->SwapWASD)
                {
                    PushKeySwapCallback( Message );
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
                    PushKeySwapCallback( Message );
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

                // Request size of the raw input buffer to dwSize
                GetRawInputData(
                    (HRAWINPUT)Message->lParam, 
                    RID_INPUT, 
                    NULL, 
                    &dwSize, 
                    sizeof(RAWINPUTHEADER)
                    );
         
                // allocate buffer for input data
                buffer = (RAWINPUT*)HeapAlloc(PushProcessHeap, 0, dwSize);
         
                if (GetRawInputData((HRAWINPUT)Message->lParam, RID_INPUT, buffer, &dwSize, sizeof(RAWINPUTHEADER)))
                {
                    // if this is keyboard message and WM_KEYDOWN, process the key
                    if(buffer->header.dwType == RIM_TYPEKEYBOARD 
                        && buffer->data.keyboard.Message == WM_KEYDOWN)
                    {
                        MenuKeyboardHook(buffer->data.keyboard.VKey);
                    }
                }
         
                // free the buffer
                HeapFree(PushProcessHeap, 0, buffer);
            } break;
    }

    return TRUE;
}


BOOL
WINAPI
PeekMessageWHook( LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg )
{
    BOOL result;

    result = PushPeekMessageW( lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg );

    if (result)
    {
        result = MessageHook( lpMsg );
    }
    
    return result;
}


BOOL
WINAPI
PeekMessageAHook( LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg )
{
    BOOL result;

    result = PushPeekMessageA( lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg );
    
    if (result)
    {
        result = MessageHook( lpMsg );
    }

    return result;
}


BOOL __stdcall DllMain( HINSTANCE Instance, ULONG fdwReason, LPVOID lpReserved )
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            void *sectionHandle;
            DEVMODE devMode;
            SlHookManager hookManager;
            OV_HOOK_PARAMS hookParams = {0};

            sectionHandle = OpenFileMappingW( FILE_MAP_ALL_ACCESS, FALSE, PUSH_SECTION_NAME );

            PushSharedMemory = (PUSH_SHARED_MEMORY *) MapViewOfFile(
                                    sectionHandle,
                                    FILE_MAP_ALL_ACCESS,
                                    NULL,
                                    NULL,
                                    sizeof(PUSH_SHARED_MEMORY)
                                    );

            hEvent = OpenEventW( SYNCHRONIZE, FALSE, L"Global\\" PUSH_IMAGE_EVENT_NAME );
            
            hookParams.RenderFunction = RnRender;
            
            if (PushSharedMemory->ForceVsync)
                hookParams.ForceVsync = TRUE;

            OvCreateOverlayEx( &hookParams );
            CreateThread(0, 0, &MonitorThread, 0, 0, 0);
            
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

            PushRefreshRate = devMode.dmDisplayFrequency;
            PushAcceptableFps = PushRefreshRate - 5;
            PushProcessHeap = GetProcessHeap();

            PushPeekMessageW = (TYPE_PeekMessageW) hookManager.DetourApi(
                L"user32.dll", 
                "PeekMessageW", 
                (BYTE*) PeekMessageWHook
                );

            PushPeekMessageA = (TYPE_PeekMessageA) hookManager.DetourApi(
                L"user32.dll", 
                "PeekMessageA", 
                (BYTE*) PeekMessageAHook
                );

        } break;

    case DLL_PROCESS_DETACH:
        {


        } break;
    }

    return TRUE;
}
