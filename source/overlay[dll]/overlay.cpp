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
void* PushImageEvent;
BOOLEAN dxgiHooked              = FALSE;
BOOLEAN g_DXGI                  = FALSE;
ThreadMonitor* PushThreadMonitor;
HANDLE PushProcessHeap;
CRITICAL_SECTION OvCriticalSection;
extern HANDLE RenderThreadHandle;


VOID Log(const wchar_t* Format, ...)
{
    wchar_t buffer[260];
    wchar_t output[260] = L"[OVRENDER] ";
    va_list _Arglist;

    _crt_va_start(_Arglist, Format);
    _vswprintf_c_l(buffer, 260, Format, NULL, _Arglist);
    _crt_va_end(_Arglist);

    wcsncat(output, buffer, 260);
    OutputDebugStringW(output);
}


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
    OutputDebugStringW(L"[OVRENDER] CreateOverlay()");
    OV_HOOK_PARAMS hookParams = { 0 };

    hookParams.RenderFunction = RnRender;

    if (PushSharedMemory->VsyncOverrideMode == PUSH_VSYNC_FORCE_ON)
    {
        OutputDebugStringW(L"[OVRENDER] VSYNC_FORCE_ON");
        hookParams.VsyncOverrideMode = VSYNC_FORCE_ON;
    }
        
    else if (PushSharedMemory->VsyncOverrideMode == PUSH_VSYNC_FORCE_OFF)
        hookParams.VsyncOverrideMode = VSYNC_FORCE_OFF;
    else
    {
        OutputDebugStringW(L"[OVRENDER] VSYNC_FORCE_UNC");
    }

    OvCreateOverlayEx(&hookParams);
}


extern HHOOK KeyboardHookHandle;


ULONG __stdcall MonitorThread( LPVOID Params )
{
    while (!RenderThreadHandle)
    {
        Sleep(500);
    }

    Log(L"RenderThreadId %i", GetThreadId(RenderThreadHandle));

    if (PushSharedMemory->KeyboardHookType == KEYBOARD_HOOK_AUTO)
    {
        Keyboard_Hook(KEYBOARD_HOOK_MESSAGE);

        if (GetModuleHandleW(L"dinput8.dll"))
        {
            Keyboard_Hook(KEYBOARD_HOOK_RAW);
        }
    }
    else
    {
        Keyboard_Hook(PushSharedMemory->KeyboardHookType);
    }

    while (PushImageEvent)
    {
        WaitForSingleObject(PushImageEvent, INFINITE);

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
    static HANDLE pipeHandle = INVALID_HANDLE_VALUE;
    DWORD dwWritten;

    if (pipeHandle == INVALID_HANDLE_VALUE)
    {
        pipeHandle = CreateFile(TEXT("\\\\.\\pipe\\Push"),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
    }
    
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


        //CloseHandle(pipeHandle);
    }
    else
    {
        Log(L"CallPipe failed!");
    }
}


VOID* OpenSection( WCHAR* SectionName, SIZE_T SectionSize )
{
    void *sectionHandle;

    sectionHandle = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SectionName
        );

    return MapViewOfFile(
        sectionHandle,
        FILE_MAP_ALL_ACCESS,
        NULL,
        NULL,
        SectionSize
        );
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
            
            DEVMODE devMode;

            PushSharedMemory = (PUSH_SHARED_MEMORY *)OpenSection(
                PUSH_SECTION_NAME, 
                sizeof(PUSH_SHARED_MEMORY)
                );

            PushImageEvent = OpenEventW(
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
            AcceptableFps = PushRefreshRate - 5;
            PushProcessHeap = GetProcessHeap();

        } break;

    case DLL_PROCESS_DETACH:
        {


        } break;
    }

    return TRUE;
}
