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
HANDLE ImageMonitoringThreadHandle;


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
    OV_HOOK_PARAMS hookParams = { 0 };

    hookParams.RenderFunction = RnRender;

    if (PushSharedMemory->VsyncOverrideMode == PUSH_VSYNC_FORCE_ON)
    {
        hookParams.VsyncOverrideMode = VSYNC_FORCE_ON;
    }
    else if (PushSharedMemory->VsyncOverrideMode == PUSH_VSYNC_FORCE_OFF)
        hookParams.VsyncOverrideMode = VSYNC_FORCE_OFF;

    hookParams.FontName = PushSharedMemory->FontName;
    hookParams.FontBold = PushSharedMemory->FontBold;

    OvCreateOverlayEx(&hookParams);
}


extern HHOOK KeyboardHookHandle;


VOID InitializeKeyboardHook()
{
    if (PushSharedMemory->KeyboardHookType == KEYBOARD_HOOK_AUTO)
    {
        Keyboard_Hook(KEYBOARD_HOOK_MESSAGE);

        // Check for DirectInput library
        if (GetModuleHandleW(L"dinput8.dll"))
        {
            Keyboard_Hook(KEYBOARD_HOOK_RAW);
        }
    }
    else
    {
        Keyboard_Hook(PushSharedMemory->KeyboardHookType);
    }
}


ULONG __stdcall ImageMonitorThread(LPVOID Params)
{
    while (PushImageEvent)
    {
        WaitForSingleObject(PushImageEvent, INFINITE);
        CreateOverlay();
    }

    return NULL;
}


VOID StopImageMonitoring()
{
    TerminateThread(ImageMonitoringThreadHandle, 0);
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


VOID CallPipe( BYTE* CommandBuffer, UINT32 CommandBufferSize, UINT16* Output )
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
            CommandBuffer,
            CommandBufferSize,
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
HINSTANCE OverlayInstance;
HHOOK Hook;
WCHAR ModuleName[260];

LRESULT CALLBACK OverlayCBTProc(
    _In_ int    nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return CallNextHookEx(Hook, nCode, wParam, lParam);
}
extern "C" __declspec(dllexport) VOID InstallOverlayHook()
{
    Hook = SetWindowsHookExA(WH_CBT, OverlayCBTProc, OverlayInstance, 0);
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
            wchar_t modulePath[260];
            wchar_t *slash;

            GetModuleFileNameW(NULL, modulePath, 260);

            slash = wcsrchr(modulePath, L'\\');
            wcscpy(ModuleName, slash + 1);

            Log(L"Dropship arrived successfully on %s", ModuleName);

            OverlayInstance = Instance;

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
            
            ImageMonitoringThreadHandle = CreateThread(
                0, 0, 
                &ImageMonitorThread, 
                0, 0, 0
                );

            PushProcessHeap = GetProcessHeap();

        } break;

    case DLL_PROCESS_DETACH:
        {
            Log(L"Dropship leaving %s ...", ModuleName);
            DestroyOverlay();
            Keyboard_UnHook();

        } break;
    }

    return TRUE;
}
