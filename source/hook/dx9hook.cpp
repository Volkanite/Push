#include <Windows.h>
#include <d3d9.h>
#include <slmodule.h>
#include <detourxs.h>

#include "dx9hook.h"


typedef struct _HOOK_PARAMS
{

    D3DPRESENT_PARAMETERS*  presentationParameters;
    VOID*   device;

} HOOK_PARAMS;
/*Callbacks*/



typedef IDirect3D9* (__stdcall *TYPE_Direct3DCreate9)(
    UINT32 SDKVersion);

typedef HRESULT (__stdcall *TYPE_Direct3DCreate9Ex)(
  _In_   UINT SDKVersion,
  _Out_  IDirect3D9Ex **ppD3D
  );

typedef LONG (__stdcall* TYPE_IDirect3D9_GetAdapterDisplayMode)(
    IDirect3D9* Object,
    UINT32 Adapter,
    D3DDISPLAYMODE* pMode
    );

typedef LONG (__stdcall* TYPE_IDirect3D9_CreateDevice)(
    IDirect3D9 *d3dObj,
    UINT32 Adapter,
    UINT32 DeviceType,
    VOID* hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS *pPresentationParameters,
    IDirect3DDevice9 **ppReturnedDeviceInterface);

//

typedef HRESULT(__stdcall *TYPE_IDirect3DDevice9_TestCooperativeLevel)(
    IDirect3DDevice9* pDevice
    );

typedef LONG (__stdcall *TYPE_IDirect3DDevice9_Present) (
    IDirect3DDevice9* pDevice,
    const RECT* pSourceRect,
    const RECT* pDestRect,
    VOID* hDestWindowOverride,
    const RGNDATA* pDirtyRegion
);

typedef LONG (__stdcall *TYPE_IDirect3DDevice9_Reset) (
    IDirect3DDevice9* pDevice,
    D3DPRESENT_PARAMETERS* pPresentationParameters
);

typedef LONG (__stdcall *TYPE_IDirect3DDevice9_GetSwapChain)(
    IDirect3DDevice9* device,
    UINT32 iSwapChain,
    IDirect3DSwapChain9** pSwapChain
    );


//IDirect3DSwapChain9

typedef LONG (__stdcall *TYPE_IDirect3DSwapChain9_Present)(
    IDirect3DSwapChain9* OBJECT_IDirect3DSwapChain9,
    const RECT* pSourceRect,
    const RECT* pDestRect,
    VOID* hDestWindowOverride,
    const RGNDATA* pDirtyRegion,
    unsigned long dwFlags
    );

typedef LONG (__stdcall *TYPE_IDirect3DSwapChain9_GetDevice)(
    IDirect3DSwapChain9* OBJECT_IDirect3DSwapChain9,
    IDirect3DDevice9** ppDevice
    );


//
typedef HRESULT (__stdcall *TYPE_IDirect3DDevice9Ex_PresentEx)( 
    IDirect3DDevice9Ex* Device, 
    CONST RECT* SourceRect, 
    CONST RECT* DestRect, 
    HWND DestWindowOverride,
    CONST RGNDATA* DirtyRegion,
    DWORD Flags
    );

typedef HRESULT (__stdcall *TYPE_IDirect3DDevice9Ex_ResetEx)(
    IDirect3DDevice9* pDevice,
    D3DPRESENT_PARAMETERS *pPresentationParameters,
    D3DDISPLAYMODEEX *pFullscreenDisplayMode
    );


TYPE_Direct3DCreate9                        HkDirect3DCreate9;
TYPE_Direct3DCreate9Ex                      Dx9Hook_Direct3DCreate9Ex;
TYPE_IDirect3D9_CreateDevice                Dx9Hook_IDirect3D9_CreateDevice;
TYPE_IDirect3DDevice9_TestCooperativeLevel  D3D9Hook_IDirect3DDevice9_TestCooperativeLevel;
TYPE_IDirect3DDevice9_GetSwapChain          IDirect3DDevice9_GetSwapChain;
TYPE_IDirect3DSwapChain9_Present            Dx9Hook_IDirect3DSwapChain9_Present;
TYPE_IDirect3DSwapChain9_GetDevice          IDirect3DSwapChain9_GetDevice;
TYPE_IDirect3DDevice9_Present               Dx9Hook_IDirect3DDevice9_Present;
TYPE_IDirect3DDevice9_Reset                 Dx9Hook_IDirect3DDevice9_Reset;
TYPE_IDirect3DDevice9Ex_PresentEx           Dx9Hook_IDirect3DDevice9Ex_PresentEx;
TYPE_IDirect3DDevice9Ex_ResetEx             Dx9Hook_IDirect3DDevice9Ex_ResetEx;

DX9HOOK_PRESENT_CALLBACK    Dx9Hook_Present;
DX9HOOK_RESET_CALLBACK      Dx9Hook_Reset;
DX9HOOK_RESET_CALLBACK      Dx9Hook_CreateDevice;

HOOK_PARAMS hookParams;
D3DPRESENT_PARAMETERS PresentParams;
BOOLEAN D3D9Hook_ForceReset = FALSE;


HRESULT STDMETHODCALLTYPE Dx9Hook_IDirect3DDevice9_Present_Detour(
    IDirect3DDevice9* Device,
    CONST RECT* SourceRect,
    CONST RECT* DestRect,
    VOID* DestWindowOverride,
    CONST RGNDATA* DirtyRegion
    )
{
    HRESULT result;

    Dx9Hook_Present( Device );

    result = Dx9Hook_IDirect3DDevice9_Present(
        Device,
        SourceRect,
        DestRect,
        DestWindowOverride,
        DirtyRegion
        );

    if (D3D9Hook_ForceReset)
    {
        result = D3DERR_DEVICELOST;
    }

    return result;
}


HRESULT STDMETHODCALLTYPE Dx9Hook_IDirect3DDevice9Ex_PresentEx_Detour( 
    IDirect3DDevice9Ex* Device, 
    CONST RECT* SourceRect, 
    CONST RECT* DestRect, 
    HWND DestWindowOverride,
    CONST RGNDATA* DirtyRegion,
    DWORD Flags
    )
{
    HRESULT result;

    Dx9Hook_Present( Device );

    result = Dx9Hook_IDirect3DDevice9Ex_PresentEx( 
        Device, 
        SourceRect, 
        DestRect, 
        DestWindowOverride,
        DirtyRegion,
        Flags
        );

    if (D3D9Hook_ForceReset)
    {
        result = D3DERR_DEVICELOST;
    }

    return result;
}


HRESULT STDMETHODCALLTYPE Dx9Hook_IDirect3DSwapChain9_Present_Detour(
    IDirect3DSwapChain9 *Swap,
    CONST RECT* SourceRect,
    CONST RECT* DestRect,
    VOID* DestWindowOverride,
    CONST RGNDATA* DirtyRegion,
    DWORD Flags
    )
{
    static IDirect3DDevice9 *device = NULL;
    HRESULT result;

    if (!device)
    {   
        Swap->GetDevice(&device);
    }

    Dx9Hook_Present( device );

    result = Dx9Hook_IDirect3DSwapChain9_Present(
        Swap,
        SourceRect,
        DestRect,
        DestWindowOverride,
        DirtyRegion,
        Flags
        );

    if (D3D9Hook_ForceReset)
    {
        result = D3DERR_DEVICELOST;
    }

    return result;
}


HRESULT STDMETHODCALLTYPE Dx9Hook_IDirect3DDevice9_Reset_Detour(
    IDirect3DDevice9* Device,
    D3DPRESENT_PARAMETERS* PresentationParameters
    )
{
    HRESULT result;
    
    Dx9Hook_Reset( PresentationParameters );

    result = Dx9Hook_IDirect3DDevice9_Reset( Device, PresentationParameters );

    D3D9Hook_ForceReset = FALSE;

    return result;
}


HRESULT STDMETHODCALLTYPE Dx9Hook_IDirect3DDevice9Ex_ResetEx_Detour(
    IDirect3DDevice9* Device,
    D3DPRESENT_PARAMETERS* PresentationParameters,
    D3DDISPLAYMODEEX* FullscreenDisplayMode
    )
{
    HRESULT result;
    
    Dx9Hook_Reset( PresentationParameters );

    result = Dx9Hook_IDirect3DDevice9Ex_ResetEx( 
        Device, 
        PresentationParameters, 
        FullscreenDisplayMode 
        );

    D3D9Hook_ForceReset = FALSE;

    return result;
}


HRESULT STDMETHODCALLTYPE Dx9Hook_IDirect3D9_CreateDevice_Detour(
    IDirect3D9* D3D9,
    UINT32 Adapter,
    UINT32 DeviceType,
    VOID* FocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS *PresentationParameters,
    IDirect3DDevice9 **ReturnedDeviceInterface
    )
{
    HRESULT result;

    Dx9Hook_CreateDevice( PresentationParameters );

    result = Dx9Hook_IDirect3D9_CreateDevice(
        D3D9,
        Adapter,
        DeviceType,
        FocusWindow,
        BehaviorFlags,
        PresentationParameters,
        ReturnedDeviceInterface
        );

    return result;
}


HRESULT STDMETHODCALLTYPE IDirect3DDevice9_TestCooperativeLevel_Detour( IDirect3DDevice9* Device )
{
    HRESULT result;
        
    result = D3D9Hook_IDirect3DDevice9_TestCooperativeLevel(Device);

    if (D3D9Hook_ForceReset)
    {
        result = D3DERR_DEVICENOTRESET;
    }
        
    return result;
}


VOID Dx9Hook_Initialize( D3D9HOOK_PARAMS* HookParams )
{
    VOID *base = NULL, **vmt;
    DetourXS *detour;
    HRESULT result;
    IDirect3D9Ex *d3d9ex;
    D3DDISPLAYMODE d3dDisplayMode;
    D3DPRESENT_PARAMETERS presentationParameters;
    IDirect3DDevice9Ex *deviceEx;
    IDirect3DSwapChain9 *swap;

    Dx9Hook_Present = HookParams->PresentCallback;
    Dx9Hook_Reset = HookParams->ResetCallback;
    Dx9Hook_CreateDevice = HookParams->CreateDeviceCallback;

    // Get module handle
    base = SlLoadLibrary(L"d3d9.dll");
   
    Dx9Hook_Direct3DCreate9Ex = (TYPE_Direct3DCreate9Ex)GetProcAddress(
        (HMODULE)base, 
        "Direct3DCreate9Ex"
        );

    if (Dx9Hook_Direct3DCreate9Ex == NULL)
        return;

    result = Dx9Hook_Direct3DCreate9Ex( D3D_SDK_VERSION, &d3d9ex );

    if (FAILED(result))
        return;

    ZeroMemory(&presentationParameters, sizeof(D3DPRESENT_PARAMETERS));

    d3d9ex->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3dDisplayMode );

    presentationParameters.Windowed                 = TRUE;
    presentationParameters.SwapEffect               = D3DSWAPEFFECT_DISCARD;
    presentationParameters.BackBufferFormat         = d3dDisplayMode.Format;

    result = d3d9ex->CreateDeviceEx(
        D3DADAPTER_DEFAULT, 
        D3DDEVTYPE_HAL, 
        GetDesktopWindow(),
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &presentationParameters, 
        NULL, 
        &deviceEx
        );

    if (FAILED(result))
        return;
    
    vmt = (VOID**) d3d9ex;
    vmt = (VOID**) vmt[0];

    detour = new DetourXS(vmt[16], Dx9Hook_IDirect3D9_CreateDevice_Detour);
    Dx9Hook_IDirect3D9_CreateDevice = (TYPE_IDirect3D9_CreateDevice)detour->GetTrampoline();

    vmt = (VOID**) deviceEx;
    vmt = (VOID**) vmt[0];

    detour = new DetourXS(vmt[3], IDirect3DDevice9_TestCooperativeLevel_Detour);
    D3D9Hook_IDirect3DDevice9_TestCooperativeLevel = (TYPE_IDirect3DDevice9_TestCooperativeLevel)detour->GetTrampoline();

    detour = new DetourXS(vmt[17], Dx9Hook_IDirect3DDevice9_Present_Detour);
    Dx9Hook_IDirect3DDevice9_Present = (TYPE_IDirect3DDevice9_Present)detour->GetTrampoline();

    detour = new DetourXS(vmt[16], Dx9Hook_IDirect3DDevice9_Reset_Detour);
    Dx9Hook_IDirect3DDevice9_Reset = (TYPE_IDirect3DDevice9_Reset)detour->GetTrampoline();

    detour = new DetourXS(vmt[121], Dx9Hook_IDirect3DDevice9Ex_PresentEx_Detour);
    Dx9Hook_IDirect3DDevice9Ex_PresentEx = (TYPE_IDirect3DDevice9Ex_PresentEx)detour->GetTrampoline();

    detour = new DetourXS(vmt[132], Dx9Hook_IDirect3DDevice9Ex_ResetEx_Detour);
    Dx9Hook_IDirect3DDevice9Ex_ResetEx = (TYPE_IDirect3DDevice9Ex_ResetEx)detour->GetTrampoline();

    result = deviceEx->GetSwapChain(0, &swap);

    if (FAILED(result))
        return;

    vmt = (VOID**) swap;
    vmt = (VOID**) vmt[0];

    detour = new DetourXS(vmt[3], Dx9Hook_IDirect3DSwapChain9_Present_Detour);
    Dx9Hook_IDirect3DSwapChain9_Present = (TYPE_IDirect3DSwapChain9_Present)detour->GetTrampoline();

    deviceEx->Release();
    d3d9ex->Release();
}
