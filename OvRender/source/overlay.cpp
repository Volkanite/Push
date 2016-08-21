#include <windows.h>
#include <OvRender.h>

#include "d3d8\d3d8overlay.h"
#include "d3d9\d3d9overlay.h"
#include "dxgi\dxgioverlay.h"
#include "ddraw\ddrawoverlay.h"


Dx8Overlay*     OvDx8Overlay;
Dx9Overlay*     D3D9Overlay;
DxgiOverlay*    DXGIOverlay;
DDrawOverlay*   DirectDrawOverlay;
extern CRITICAL_SECTION OvCriticalSection;


OvOverlay::OvOverlay()
{
    Line = 0;
    VsyncOverrideMode = VSYNC_UNCHANGED;
}


VOID
OvOverlay::Render()
{
    Line = 0;

    Begin();
    UserRenderFunction( this );
    End();
}


ULONG __stdcall CreateOverlay( LPVOID Param )
{
    EnterCriticalSection(&OvCriticalSection);

    OV_HOOK_PARAMS *hookParams = (OV_HOOK_PARAMS*) Param;
    OvOverlay *overlay = NULL;
    BOOLEAN d3d8, d3d9, dxgi, ddraw;

    if (GetModuleHandleW(L"d3d8.dll"))
    {
        OutputDebugStringW(L"GetModuleHandle(\"d3d8.dll\") returned TRUE");
        d3d8 = TRUE;
    }
    else
    {
        OutputDebugStringW(L"GetModuleHandle(\"d3d8.dll\") returned FALSE");
        d3d8 = FALSE;
    }

    if (GetModuleHandleW(L"d3d9.dll"))
    {
        OutputDebugStringW(L"GetModuleHandle(\"d3d9.dll\") returned TRUE");
        d3d9 = TRUE;
    }
    else
    {
        OutputDebugStringW(L"GetModuleHandle(\"d3d9.dll\") returned FALSE");
        d3d9 = FALSE;
    }

    if (GetModuleHandleW(L"dxgi.dll"))
    {
        OutputDebugStringW(L"GetModuleHandle(\"dxgi.dll\") returned TRUE");
        dxgi = TRUE;
    }
    else
    {
        OutputDebugStringW(L"GetModuleHandle(\"dxgi.dll\") returned FALSE");
        dxgi = FALSE;
    }

    if (GetModuleHandleW(L"ddraw.dll"))
    {
        OutputDebugStringW(L"GetModuleHandle(\"ddraw.dll\") returned TRUE");
        ddraw = TRUE;
    }
    else
    {
        OutputDebugStringW(L"GetModuleHandle(\"ddraw.dll\") returned FALSE");
        ddraw = FALSE;
    }

    if (d3d8 && OvDx8Overlay == NULL)
    {
        OutputDebugStringW(L"Hooking d3d8...");
        OvDx8Overlay = new Dx8Overlay( hookParams->RenderFunction );
        overlay = OvDx8Overlay;
    }

    if (d3d9 && D3D9Overlay == NULL)
    {
        OutputDebugStringW(L"Hooking d3d9...");
        D3D9Overlay = new Dx9Overlay( hookParams->RenderFunction );
        overlay = D3D9Overlay;
    }

    if (dxgi && DXGIOverlay == NULL)
    {
        OutputDebugStringW(L"Hooking dxgi...");
        DXGIOverlay = new DxgiOverlay(hookParams->RenderFunction);
        overlay = DXGIOverlay;
    }

    if (ddraw && DirectDrawOverlay == NULL)
    {
        OutputDebugStringW(L"Hooking ddraw...");
        DirectDrawOverlay = new DDrawOverlay(hookParams->RenderFunction);
        overlay = DirectDrawOverlay;
    }

    if (overlay)
        overlay->VsyncOverrideMode = hookParams->VsyncOverrideMode;

    LeaveCriticalSection(&OvCriticalSection);

    return NULL;
}


VOID
OvCreateOverlay( OV_RENDER RenderFunction )
{
    OV_HOOK_PARAMS hookParams = {0};

    hookParams.RenderFunction = RenderFunction;

    OvCreateOverlayEx(&hookParams);
}


VOID
OvCreateOverlayEx( OV_HOOK_PARAMS* HookParameters )
{
    OV_HOOK_PARAMS *hookParams;

    hookParams = (OV_HOOK_PARAMS*) HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(OV_HOOK_PARAMS)
        );

    hookParams->RenderFunction = HookParameters->RenderFunction;
    hookParams->VsyncOverrideMode = HookParameters->VsyncOverrideMode;

    CreateThread(0, 0, &CreateOverlay, hookParams, 0, 0);
}
