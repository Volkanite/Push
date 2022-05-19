#include <Windows.h>
#include <stdio.h>

#include "overlay.h"
#include <OvRender.h>
#include "render.h"
#include "thread.h"
#include "menu.h"
#include "kbhook.h"
#include <hexus.h>


CHAR *pszModuleName;
PUSH_SHARED_MEMORY   *PushSharedMemory;
void* PushImageEvent;
BOOLEAN dxgiHooked              = FALSE;
BOOLEAN g_DXGI                  = FALSE;
ThreadMonitor* PushThreadMonitor;
HANDLE PushProcessHeap;
CRITICAL_SECTION OvCriticalSection;
extern HANDLE RenderThreadHandle;
extern BOOLEAN SpoofControllerType;
HANDLE ImageMonitoringThreadHandle;


VOID Log(const wchar_t* Format, ...)
{
    wchar_t buffer[260];
    wchar_t output[260] = L"[OVERLAY32] ";
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

DWORD GetOverlayMask()
{
    DWORD mask = 0;

    if (GetModuleHandleW(L"d3d8.dll"))
        mask |= OV_D3D8;

    if (GetModuleHandleW(L"d3d9.dll"))
        mask |= OV_D3D9;

    if (GetModuleHandleW(L"dxgi.dll"))
        mask |= OV_DXGI;

    if (GetModuleHandleW(L"ddraw.dll"))
        mask |= OV_DDRAW;

    return mask;
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


VOID* DetourApi( WCHAR* dllName, CHAR* apiName, BYTE* NewFunction )
{
    BYTE *functionStart = NULL;
    HMODULE moduleHandle;
    DWORD address = 0;
    DETOUR_PROPERTIES detour;

    // Get the API address
    moduleHandle = GetModuleHandleW(dllName);
    address = (DWORD)GetProcAddress(moduleHandle, apiName);

    if (!address || !NewFunction)
        return NULL;

    functionStart = (BYTE*)address;
    DetourCreate(functionStart, NewFunction, &detour);

     return detour.Trampoline;
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

        CloseHandle(pipeHandle);
        pipeHandle = INVALID_HANDLE_VALUE;
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
BOOLEAN Guard;
void HookDirectInput();


LRESULT CALLBACK OverlayCBTProc(
    _In_ int    nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
	if (nCode == HCBT_KEYSKIPPED)
	{
		if ((0x80000000 & lParam) == 0)//key down
		{
			Menu_KeyboardHook(wParam);
		}
	}

	if (nCode == HCBT_ACTIVATE || nCode == HCBT_CREATEWND || nCode == HCBT_SETFOCUS)
	{
		/*if (nCode == HCBT_CREATEWND)
			Log(L"CreateWindow(0x%X)", wParam);

		if (nCode == HCBT_ACTIVATE)
			Log(L"ActivateWindow(0x%X)", wParam);

		if (nCode == HCBT_SETFOCUS)
			Log(L"SetFocus(0x%X)", wParam);*/

		if (!RenderThreadHandle && !Guard)
		{
			Guard = TRUE;
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
			hookParams.InterfaceFlags = GetOverlayMask();
			hookParams.WindowHandle = (HWND)wParam;

			OvCreateOverlay(&hookParams);
			//HookDirectInput();
			Guard = FALSE;
		}
	}
    
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

            COMMAND_HEADER cmdBuffer;
            cmdBuffer.CommandIndex = CMD_NOTIFY;
            cmdBuffer.ProcessId = GetCurrentProcessId();
            WORD result;
            CallPipe((BYTE*)&cmdBuffer, sizeof(cmdBuffer), &result);

            if (PushSharedMemory->SpoofControllerType)
                SpoofControllerType = TRUE;

            PushImageEvent = OpenEventW(
                SYNCHRONIZE,
                FALSE,
                L"Global\\"
                PUSH_IMAGE_EVENT_NAME
                );

            InitializeCriticalSection(&OvCriticalSection);

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
