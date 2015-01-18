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


BOOLEAN
ProcessMessage( LPMSG lpMsg )
{
    if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_CHAR)
    {
        MenuKeyboardCallback( lpMsg->wParam );

        if (PushSharedMemory->DisableRepeatKeys)
        {
            int repeatCount = (lpMsg->lParam & 0x40000000);

            if (repeatCount) 
                return FALSE;
        }
    }

    // Game uses raw input?
    if (lpMsg->message == WM_INPUT)
    {
        UINT dwSize;
        RAWINPUT *buffer;

        // request size of the raw input buffer to dwSize
        GetRawInputData(
            (HRAWINPUT)lpMsg->lParam, 
            RID_INPUT, 
            NULL, 
            &dwSize, 
            sizeof(RAWINPUTHEADER)
            );
         
        // allocate buffer for input data
        buffer = (RAWINPUT*)HeapAlloc(GetProcessHeap(), 0, dwSize);
         
        if (GetRawInputData(
            (HRAWINPUT)lpMsg->lParam, 
            RID_INPUT, 
            buffer, 
            &dwSize, 
            sizeof(RAWINPUTHEADER)))
            {
                // if this is keyboard message and WM_KEYDOWN, 
                // process the key
                if(buffer->header.dwType == RIM_TYPEKEYBOARD 
                    && buffer->data.keyboard.Message == WM_KEYDOWN)
                {
                    static DWORD time;

                    // Check if same message
                    if (lpMsg->time == time)
                        // Return if same message
                        return TRUE;

                    // Update new time
                    time = lpMsg->time;

                    MenuKeyboardCallback( 
                        buffer->data.keyboard.VKey 
                        );
                }
            }
         
            // free the buffer
            HeapFree(GetProcessHeap(), 0, buffer);
    }

    return TRUE;
}


BOOL
WINAPI
PeekMessageWHook( 
    LPMSG lpMsg, 
    HWND hWnd, 
    UINT wMsgFilterMin, 
    UINT wMsgFilterMax, 
    UINT wRemoveMsg 
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

    if (ProcessMessage( lpMsg ))
        return result;
    else
        return FALSE;
}


BOOL
WINAPI
PeekMessageAHook( 
    LPMSG lpMsg, 
    HWND hWnd, 
    UINT wMsgFilterMin, 
    UINT wMsgFilterMax, 
    UINT wRemoveMsg 
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

    if (ProcessMessage( lpMsg ))
        return result;
    else
        return FALSE;
}



BOOL __stdcall DllMain( 
    HINSTANCE 
    hinstDLL, 
    ULONG fdwReason, 
    LPVOID lpReserved 
    )
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            void *sectionHandle;
            DEVMODE devMode;
            SlHookManager hookManager;

            sectionHandle = OpenFileMappingW( 
                FILE_MAP_ALL_ACCESS, 
                FALSE, 
                PUSH_SECTION_NAME 
                );

            PushSharedMemory = (PUSH_SHARED_MEMORY *) MapViewOfFile(
                                    sectionHandle,
                                    FILE_MAP_ALL_ACCESS,
                                    NULL,
                                    NULL,
                                    sizeof(PUSH_SHARED_MEMORY)
                                    );

            hEvent = OpenEventW(
                SYNCHRONIZE, 
                FALSE, 
                L"Global\\" 
                PUSH_IMAGE_EVENT_NAME
                );
            
            OvCreateOverlay( RnRender );
            CreateThread(0, 0, &MonitorThread, 0, 0, 0);
            
            EnumDisplaySettings(
                NULL, 
                ENUM_CURRENT_SETTINGS, 
                &devMode
                );

            PushRefreshRate = devMode.dmDisplayFrequency;
            PushAcceptableFps = PushRefreshRate - 5;

            PushPeekMessageW = (TYPE_PeekMessageW) 
                hookManager.DetourApi(
                L"user32.dll", 
                "PeekMessageW", 
                (BYTE*) PeekMessageWHook
                );

            PushPeekMessageA = (TYPE_PeekMessageA) 
                hookManager.DetourApi(
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
