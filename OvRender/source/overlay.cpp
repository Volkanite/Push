#include <windows.h>
#include <OvRender.h>

#include "d3d8\d3d8overlay.h"
#include "d3d9\d3d9overlay.h"
#include "dxgi\dxgioverlay.h"
#include "ddraw\ddrawoverlay.h"


Dx8Overlay*     D3D8Overlay;
Dx9Overlay*     D3D9Overlay;
DxgiOverlay*    DXGIOverlay;
DDrawOverlay*   DirectDrawOverlay;

//UINT32 BackBufferWidth;
//UINT32 BackBufferHeight;
OV_GRAPHICS_API GraphicsApi;

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


VOID SetOverlayProperties( OvOverlay* Overlay, OV_HOOK_PARAMS* HookParameters )
{
    Overlay->VsyncOverrideMode = HookParameters->VsyncOverrideMode;
    Overlay->FontProperties.Name = HookParameters->FontName;
    Overlay->FontProperties.Bold = HookParameters->FontBold;
    Overlay->FontProperties.Size = 10;
}


ULONG __stdcall CreateOverlayInternal( LPVOID Param )
{
    EnterCriticalSection(&OvCriticalSection);

    OV_HOOK_PARAMS *hookParams = (OV_HOOK_PARAMS*) Param;
    BOOLEAN d3d8, d3d9, dxgi, ddraw;

    d3d8 = d3d9 = dxgi = ddraw = FALSE;

    if (GetModuleHandleW(L"d3d8.dll"))
    {
        d3d8 = TRUE;
    }

    if (GetModuleHandleW(L"d3d9.dll"))
    {
        d3d9 = TRUE;
    }

    if (GetModuleHandleW(L"dxgi.dll"))
    {
        dxgi = TRUE;
    }

    if (GetModuleHandleW(L"ddraw.dll"))
    {
        ddraw = TRUE;
    }

    if (d3d8 && D3D8Overlay == NULL)
    {
        Log(L"Hooking d3d8...");
        D3D8Overlay = new Dx8Overlay(hookParams->RenderFunction);
    }

    if (d3d9 && D3D9Overlay == NULL)
    {
        Log(L"Hooking d3d9...");
        D3D9Overlay = new Dx9Overlay( hookParams->RenderFunction );
        SetOverlayProperties(D3D9Overlay, hookParams);
    }

    if (dxgi && DXGIOverlay == NULL)
    {
        Log(L"Hooking dxgi...");
        DXGIOverlay = new DxgiOverlay(hookParams->RenderFunction);
        SetOverlayProperties(DXGIOverlay, hookParams);
    }

    if (ddraw && DirectDrawOverlay == NULL)
    {
        Log(L"Hooking ddraw...");
        DirectDrawOverlay = new DDrawOverlay(hookParams->RenderFunction);
    }   

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


VOID OvCreateOverlayEx( OV_HOOK_PARAMS* HookParameters )
{
    OV_HOOK_PARAMS *hookParams;
    
    hookParams = (OV_HOOK_PARAMS*) HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(OV_HOOK_PARAMS)
        );
    
    //hookParams->RenderFunction = HookParameters->RenderFunction;
    //hookParams->VsyncOverrideMode = HookParameters->VsyncOverrideMode;

    memcpy(hookParams, HookParameters, sizeof(OV_HOOK_PARAMS));

    CreateThread(0, 0, &CreateOverlayInternal, hookParams, 0, 0);
}


VOID DestroyOverlay()
{
    if (D3D8Overlay)
        D3D8Overlay->~Dx8Overlay();

    if (D3D9Overlay)
        D3D9Overlay->~Dx9Overlay();

    if (DXGIOverlay)
        DXGIOverlay->~DxgiOverlay();
}


DWORD FindPattern(WCHAR* Module, char pattern[], char mask[])
{
    HMODULE hModule = GetModuleHandle(Module);

    BYTE* pszPatt = (BYTE*)pattern;

    DWORD dwStart = (DWORD)hModule;

    PIMAGE_DOS_HEADER pDosHeader = PIMAGE_DOS_HEADER(hModule);

    PIMAGE_NT_HEADERS pNTHeader = PIMAGE_NT_HEADERS((LONG)hModule + pDosHeader->e_lfanew);

    PIMAGE_OPTIONAL_HEADER pOptionalHeader = &pNTHeader->OptionalHeader;

    DWORD dwLen = pOptionalHeader->SizeOfCode;

    unsigned int i = NULL;

    int iLen = (int) strlen(mask) - 1;

    for (DWORD dwRet = dwStart; dwRet < dwStart + dwLen; dwRet++)
    {
        if (*(BYTE*)dwRet == pszPatt[i] || mask[i] == '?')
        {
            if (mask[i + 1] == '\0')
                return(dwRet - iLen);
            i++;
        }
        else
            i = NULL;
    }
    return NULL;
}


DWORD64 FindPattern64(WCHAR* Module, char pattern[], char mask[])
{
    HMODULE hModule = GetModuleHandle(Module);

    BYTE* pszPatt = (BYTE*)pattern;

    DWORD64 dwStart = (DWORD64)hModule;

    PIMAGE_DOS_HEADER pDosHeader = PIMAGE_DOS_HEADER(hModule);

    PIMAGE_NT_HEADERS64 pNTHeader = PIMAGE_NT_HEADERS64((DWORD64)hModule + pDosHeader->e_lfanew);

    PIMAGE_OPTIONAL_HEADER64 pOptionalHeader = &pNTHeader->OptionalHeader;

    DWORD dwLen = pOptionalHeader->SizeOfCode;

    unsigned int i = NULL;

    int iLen = (int) strlen(mask) - 1;

    for (DWORD64 dwRet = dwStart; dwRet < dwStart + dwLen; dwRet++)
    {
        if (*(BYTE*)dwRet == pszPatt[i] || mask[i] == '?')
        {
            if (mask[i + 1] == '\0')
                return(dwRet - iLen);
            i++;
        }
        else
            i = NULL;
    }
    return NULL;
}


void ReplaceVirtualMethod(void **VTable, int Function, void *Detour)
{
    DWORD old;

    if (!VTable) return;

    VirtualProtect(&(VTable[Function]), sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    VTable[Function] = Detour;
    VirtualProtect(&(VTable[Function]), sizeof(void*), old, &old);
}

