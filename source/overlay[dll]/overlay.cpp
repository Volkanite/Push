#include <Windows.h>
#include <stdio.h>


#include "overlay.h"
#include <OvRender.h>
#include "render.h"
#include "thread.h"


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

    if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_CHAR)
    {
        int repeatCount = (lpMsg->lParam & 0x40000000);

        if (repeatCount)
            return 0;
    }

    return result;
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

    if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_CHAR)
    {
        int repeatCount = (lpMsg->lParam & 0x40000000);

        if (repeatCount)
            return 0;
    }

    return result;
}


#include <sldetours.h>
BOOL __stdcall DllMain( HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpReserved )
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            void *sectionHandle;
            DEVMODE devMode;

            sectionHandle = OpenFileMappingW( FILE_MAP_ALL_ACCESS, FALSE, PUSH_SECTION_NAME );

            PushSharedMemory = (PUSH_SHARED_MEMORY *) MapViewOfFile(
                                    sectionHandle,
                                    FILE_MAP_ALL_ACCESS,
                                    NULL,
                                    NULL,
                                    sizeof(PUSH_SHARED_MEMORY)
                                    );

            hEvent = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\" PUSH_IMAGE_EVENT_NAME);
			
            OvCreateOverlay( RnRender );
            CreateThread(0, 0, &MonitorThread, 0, 0, 0);
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

            PushRefreshRate = devMode.dmDisplayFrequency;
            PushAcceptableFps = PushRefreshRate - 5;

            if (PushSharedMemory->DisableRepeatKeys)
            {
                SlHookManager hookManager;

                PushPeekMessageW = (TYPE_PeekMessageW) hookManager.DetourApi(L"user32.dll", "PeekMessageW", (BYTE*) PeekMessageWHook);
                PushPeekMessageA = (TYPE_PeekMessageA) hookManager.DetourApi(L"user32.dll", "PeekMessageA", (BYTE*) PeekMessageAHook);
            }

        } break;

    case DLL_PROCESS_DETACH:
        {


        } break;
    }

    return TRUE;
}
