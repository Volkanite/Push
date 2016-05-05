#include <Windows.h>
#include <stdio.h>
#include <detourxs.h>

#include "overlay.h"
#include <OvRender.h>
#include "render.h"
#include "thread.h"
#include "menu.h"
#include "kbhook.h"


CHAR *pszModuleName;
PUSH_SHARED_MEMORY   *PushSharedMemory;
void            *hEvent = NULL;
BOOLEAN dxgiHooked              = FALSE;
BOOLEAN g_DXGI                  = FALSE;
ThreadMonitor* PushThreadMonitor;
HANDLE PushProcessHeap;
CRITICAL_SECTION OvCriticalSection;


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


VOID CreateOverlay()
{
    OV_HOOK_PARAMS hookParams = { 0 };

    hookParams.RenderFunction = RnRender;

    if (PushSharedMemory->VsyncOverrideMode == PUSH_VSYNC_FORCE_ON)
        hookParams.VsyncOverrideMode = VSYNC_FORCE_ON;
    else if (PushSharedMemory->VsyncOverrideMode == PUSH_VSYNC_FORCE_OFF)
        hookParams.VsyncOverrideMode = VSYNC_FORCE_OFF;

    OvCreateOverlayEx(&hookParams);
}

extern HHOOK KeyboardHookHandle;


ULONG __stdcall MonitorThread(LPVOID v)
{
    while (!KeyboardHookHandle)
    {
        Keyboard_Hook(KEYBOARD_HOOK_MESSAGE);

        Sleep(500);
    }

    while (TRUE)
    {
        WaitForSingleObject(hEvent, INFINITE);

        OutputDebugStringW(L"new image event!");
        CreateOverlay();
    }

    return NULL;
}


VOID* DetourApi( WCHAR* dllName, CHAR* apiName, BYTE* NewFunction )
{
    BYTE *functionStart = NULL;
    HMODULE moduleHandle;
    DWORD address = 0;
    DetourXS *detour;

    // Get the API address
    moduleHandle = GetModuleHandleW(dllName);
    address = (DWORD)GetProcAddress(moduleHandle, apiName);

    if (!address || !NewFunction)
        return NULL;

    functionStart = (BYTE*)address;
    detour = new DetourXS(functionStart, NewFunction);

     return detour->GetTrampoline();
}


VOID CallPipe( WCHAR* Command, UINT16* Output )
{
    HANDLE pipeHandle;
    DWORD dwWritten;

    pipeHandle = CreateFile(TEXT("\\\\.\\pipe\\Push"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (pipeHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesRead;

        WriteFile(
            pipeHandle,
            Command,
            (wcslen(Command) + 1) * sizeof(WCHAR),
            &dwWritten,
            NULL
            );

        if (Output)
        {
            //DebugBreak();
            ReadFile(pipeHandle, Output, 2, &bytesRead, NULL);
        }


        CloseHandle(pipeHandle);
    }
}


BOOL __stdcall DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG fdwReason,
    _In_ LPVOID lpReserved
    )
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            void *sectionHandle;
            DEVMODE devMode;

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

            InitializeCriticalSection(&OvCriticalSection);
            CreateOverlay();
            CreateThread(0, 0, &MonitorThread, 0, 0, 0);
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

            PushRefreshRate = devMode.dmDisplayFrequency;
            PushAcceptableFps = PushRefreshRate - 5;
            PushProcessHeap = GetProcessHeap();

        } break;

    case DLL_PROCESS_DETACH:
        {


        } break;
    }

    return TRUE;
}
