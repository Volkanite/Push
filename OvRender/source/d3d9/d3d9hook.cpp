#include <Windows.h>
#include <d3d9.h>
#include <slmodule.h>
#include <detourxs.h>

#include "d3d9hook.h"

#define DIRECTINPUT_VERSION 0x800
#include <dinput.h>

typedef struct _HOOK_PARAMS
{

    D3DPRESENT_PARAMETERS*  presentationParameters;
    VOID*   device;

} HOOK_PARAMS;
/*Callbacks*/


typedef enum _HOOK_METHOD
{
    HOOK_METHOD_DETOUR,
    HOOK_METHOD_VMT,

} HOOK_METHOD;


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

typedef HRESULT(__stdcall *TYPE_IDirect3DDevice9_BeginStateBlock)(
    IDirect3DDevice9* Device
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


/* Direct Input */

typedef HRESULT (__stdcall *TYPE_DirectInput8Create)(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN punkOuter
    );

/* IDirectInput8 */

typedef HRESULT(__stdcall *TYPE_IDirectInput8_CreateDevice)(
    IDirectInput8A* Device,
    REFGUID rguid,
    LPDIRECTINPUTDEVICE8A* lplpDirectInputDevice,
    LPUNKNOWN pUnkOuter
    );

typedef HRESULT(__stdcall *TYPE_IDirectInput8_EnumDevices)(
    IDirectInput8A* Device,
    DWORD dwDevType,
    LPDIENUMDEVICESCALLBACKA lpCallback,
    LPVOID pvRef,
    DWORD dwFlags
    );

typedef HRESULT(__stdcall *TYPE_IDirectInputDevice8_GetCapabilities)(
    IDirectInputDevice8A *device,
    LPDIDEVCAPS lpDIDevCaps
    );

typedef HRESULT(__stdcall *TYPE_IDirectInputDevice8_GetDeviceInfo)(
    IDirectInputDevice8A *device,
    LPDIDEVICEINSTANCEA pdidi
    );


TYPE_Direct3DCreate9                        HkDirect3DCreate9;
TYPE_Direct3DCreate9Ex                      Dx9Hook_Direct3DCreate9Ex;
TYPE_IDirect3D9_CreateDevice                Dx9Hook_IDirect3D9_CreateDevice;
TYPE_IDirect3DDevice9_TestCooperativeLevel  D3D9Hook_IDirect3DDevice9_TestCooperativeLevel;
TYPE_IDirect3DDevice9_GetSwapChain          IDirect3DDevice9_GetSwapChain;
TYPE_IDirect3DDevice9_BeginStateBlock       D3D9Hook_IDirect3DDevice9_BeginStateBlock;
TYPE_IDirect3DSwapChain9_Present            D3D9Hook_IDirect3DSwapChain9_Present;
TYPE_IDirect3DSwapChain9_GetDevice          IDirect3DSwapChain9_GetDevice;
TYPE_IDirect3DDevice9_Present               D3D9Hook_IDirect3DDevice9_Present;
TYPE_IDirect3DDevice9_Reset                 D3D9Hook_IDirect3DDevice9_Reset;
TYPE_IDirect3DDevice9Ex_PresentEx           Dx9Hook_IDirect3DDevice9Ex_PresentEx;
TYPE_IDirect3DDevice9Ex_ResetEx             Dx9Hook_IDirect3DDevice9Ex_ResetEx;
TYPE_DirectInput8Create                     D3D9Hook_DirectInput8Create;
TYPE_IDirectInput8_CreateDevice             D3D9Hook_IDirectInput8_CreateDevice;
TYPE_IDirectInput8_EnumDevices              D3D9Hook_IDirectInput8_EnumDevices;
TYPE_IDirectInputDevice8_GetCapabilities    D3D9Hook_IDirectInputDevice8_GetCapabilities;
TYPE_IDirectInputDevice8_GetDeviceInfo      D3D9Hook_IDirectInputDevice8_GetDeviceInfo;


DX9HOOK_PRESENT_CALLBACK    Dx9Hook_Present;
DX9HOOK_RESET_CALLBACK      Dx9Hook_Reset;
DX9HOOK_RESET_CALLBACK      Dx9Hook_CreateDevice;

HOOK_PARAMS hookParams;
D3DPRESENT_PARAMETERS PresentParams;
HOOK_METHOD D3D9Hook_HookMethod = HOOK_METHOD_DETOUR;
BOOLEAN D3D9Hook_ForceReset = FALSE;
IDirect3DDevice9*   D3D9Hook_IDirect3DDevice9;


typedef enum _PRESENT_ROUTINE{
    IDirect3DDevice9_Present,
    IDirect3DDevice9Ex_PresentEx,
    IDirect3DSwapChain9_Present
}PRESENT_ROUTINE;


DWORD FindPattern(WCHAR* Module, char pattern[], char mask[]);
DWORD64 FindPattern64(WCHAR* Module, char pattern[], char mask[]);

void ReplaceVirtualMethod(void **VTable, int Function, void *Detour);
VOID ReplaceVirtualMethods(IDirect3DDevice9* Device);
VOID Log(const wchar_t* Format, ...);


HRESULT __stdcall IDirect3DDevice9_Present_Detour(
    IDirect3DDevice9* Device,
    CONST RECT* SourceRect,
    CONST RECT* DestRect,
    VOID* DestWindowOverride,
    CONST RGNDATA* DirtyRegion
    )
{
    HRESULT result;
    static BOOLEAN init = FALSE;

    if (!init)
    {
        init = TRUE;

        Log(L"Hook called @ IDirect3DDevice9_Present");
    }

    Dx9Hook_Present( Device );

    result = D3D9Hook_IDirect3DDevice9_Present(
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


HRESULT __stdcall IDirect3DDevice9Ex_PresentEx_Detour( 
    IDirect3DDevice9Ex* Device, 
    CONST RECT* SourceRect, 
    CONST RECT* DestRect, 
    HWND DestWindowOverride,
    CONST RGNDATA* DirtyRegion,
    DWORD Flags
    )
{
    HRESULT result;
    static BOOLEAN init = FALSE;

    if (!init)
    {
        init = TRUE;

        Log(L"Hook called @ IDirect3DDevice9Ex_PresentEx");
    }

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


HRESULT __stdcall IDirect3DSwapChain9_Present_Detour(
    IDirect3DSwapChain9 *Swap,
    CONST RECT* SourceRect,
    CONST RECT* DestRect,
    VOID* DestWindowOverride,
    CONST RGNDATA* DirtyRegion,
    DWORD Flags
    )
{
    static IDirect3DDevice9 *device = NULL;
    static IDirect3DSwapChain9 *backupSwap = NULL;
    HRESULT result;
    static BOOLEAN init = FALSE;

    if (!init)
    {
        init = TRUE;

        Log(L"Hook called @ IDirect3DSwapChain9_Present");
    }
    
    if (!backupSwap || !device)
    {   
        backupSwap = Swap;
        Swap->GetDevice(&device);
    }

    if (backupSwap != Swap)
    {
        backupSwap = NULL;
        device = NULL;
        D3D9Hook_ForceReset = TRUE;
        return D3DERR_DEVICELOST;
    }

    Dx9Hook_Present( device );

    result = D3D9Hook_IDirect3DSwapChain9_Present(
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


HRESULT __stdcall IDirect3DDevice9_Reset_Detour(
    IDirect3DDevice9* Device,
    D3DPRESENT_PARAMETERS* PresentationParameters
    )
{
    HRESULT result;
    
    Dx9Hook_Reset( PresentationParameters );

    result = D3D9Hook_IDirect3DDevice9_Reset( Device, PresentationParameters );

    D3D9Hook_ForceReset = FALSE;

    return result;
}


HRESULT __stdcall Dx9Hook_IDirect3DDevice9Ex_ResetEx_Detour(
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


HRESULT __stdcall IDirect3DDevice9_TestCooperativeLevel_Detour( IDirect3DDevice9* Device )
{
    HRESULT result;

    result = D3D9Hook_IDirect3DDevice9_TestCooperativeLevel(Device);

    if (D3D9Hook_ForceReset)
    {
        result = D3DERR_DEVICENOTRESET;
    }

    return result;
}


HRESULT __stdcall IDirect3DDevice9_BeginStateBlock_Detour( IDirect3DDevice9* Device )
{
    HRESULT result;

    result = D3D9Hook_IDirect3DDevice9_BeginStateBlock( Device );

    ReplaceVirtualMethods( Device );

    return result;
}


VOID ReplaceVirtualMethods( IDirect3DDevice9* Device )
{
    VOID **vmt;
    //HRESULT result;
    //IDirect3DSwapChain9* swapChain;

    if (!Device) return;

    vmt = (VOID**)Device;
    vmt = (VOID**)vmt[0];

    // Backup
    D3D9Hook_IDirect3DDevice9_TestCooperativeLevel = (TYPE_IDirect3DDevice9_TestCooperativeLevel)vmt[3];
    D3D9Hook_IDirect3DDevice9_Reset = (TYPE_IDirect3DDevice9_Reset)vmt[16];
    D3D9Hook_IDirect3DDevice9_Present = (TYPE_IDirect3DDevice9_Present)vmt[17];
    //D3D9Hook_IDirect3DDevice9_BeginStateBlock = (TYPE_IDirect3DDevice9_BeginStateBlock)vmt[60];

    //Overwrite
    ReplaceVirtualMethod(vmt, 3, IDirect3DDevice9_TestCooperativeLevel_Detour);
    ReplaceVirtualMethod(vmt, 16, IDirect3DDevice9_Reset_Detour);
    ReplaceVirtualMethod(vmt, 17, IDirect3DDevice9_Present_Detour);
    //ReplaceVirtualMethod(vmt, 60, IDirect3DDevice9_BeginStateBlock_Detour);

    /*result = Device->GetSwapChain(0, &swapChain);

    if (result == S_OK)
    {
        vmt = (VOID**)swapChain;
        vmt = (VOID**)vmt[0];

        ReplaceVirtualMethod(vmt, 3, IDirect3DSwapChain9_Present_Detour);
    }*/
}


HRESULT __stdcall IDirect3D9_CreateDevice_Detour(
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

    if (D3D9Hook_HookMethod == HOOK_METHOD_VMT && result == S_OK)
    {
        VOID **vmt;
        HRESULT hr;
        IDirect3DSwapChain9 *swap;

        vmt = (VOID**)*ReturnedDeviceInterface;
        vmt = (VOID**)vmt[0];
        
        D3D9Hook_IDirect3DDevice9 = *ReturnedDeviceInterface;

        hr = (*ReturnedDeviceInterface)->GetSwapChain(0, &swap);

        if (hr == S_OK)
        {
            vmt = (VOID**)swap;
            vmt = (VOID**)vmt[0];

            D3D9Hook_IDirect3DSwapChain9_Present = (TYPE_IDirect3DSwapChain9_Present)vmt[3];
        }

        ReplaceVirtualMethods( *ReturnedDeviceInterface );   
    }

    return result;
}


#define PATT_CD3DHAL_VFTABLE_x86    "\xC7\x06\x00\x00\x00\x00\x8B\x00\x00\x00\x00\x00\xC7\x45\x00\x00\x00\x00\x00\x85\xFF\x0F"
#define MASK_CD3DHAL_VFTABLE_x86    "xx????x?????xx?????xxx"
#define PATT_CD3DHAL_VFTABLE_x64    "\x48\x8D\x05\x36\x0C"
#define MASK_CD3DHAL_VFTABLE_x64    "xxxxx"
#define PATT_D3D9SWAPCHAINPRESENT   "\x8B\xFF\x55\x8B\xEC\x8B\x45\x1C"
#define MASK_D3D9SWAPCHAINPRESENT   "xxxxxxxx"


DetourXS *DetourTestCooperativeLevel;
DetourXS *DetourReset;
DetourXS *DetourPresent;
DetourXS *DetourResetEx;
DetourXS *DetourPresentEx;
DetourXS *DetourSwapChainPresent;
DetourXS *Detour_DirectInput_CreateDevice;
DetourXS *Detour_DirectInput_EnumDevices;
DetourXS *Detour_DirectInputDevice_GetCapabilities;
DetourXS *Detour_DirectInputDevice_GetDeviceInfo;


VOID ApplyDetourXsHooks( IDirect3DDevice9Ex* Device )
{
    VOID **virtualMethodTable;
    
    Log(L"=> ApplyDetourXsHooks()");
    
    virtualMethodTable = (VOID**)Device;
    virtualMethodTable = (VOID**)virtualMethodTable[0];

    DetourTestCooperativeLevel = new DetourXS(virtualMethodTable[3], IDirect3DDevice9_TestCooperativeLevel_Detour);
    D3D9Hook_IDirect3DDevice9_TestCooperativeLevel = (TYPE_IDirect3DDevice9_TestCooperativeLevel)DetourTestCooperativeLevel->GetTrampoline();

    DetourReset = new DetourXS(virtualMethodTable[16], IDirect3DDevice9_Reset_Detour);
    D3D9Hook_IDirect3DDevice9_Reset = (TYPE_IDirect3DDevice9_Reset)DetourReset->GetTrampoline();

    DetourPresent = new DetourXS(virtualMethodTable[17], IDirect3DDevice9_Present_Detour);
    D3D9Hook_IDirect3DDevice9_Present = (TYPE_IDirect3DDevice9_Present)DetourPresent->GetTrampoline();

    DetourPresentEx = new DetourXS(virtualMethodTable[121], IDirect3DDevice9Ex_PresentEx_Detour);
    Dx9Hook_IDirect3DDevice9Ex_PresentEx = (TYPE_IDirect3DDevice9Ex_PresentEx)DetourPresentEx->GetTrampoline();

    DetourResetEx = new DetourXS(virtualMethodTable[132], Dx9Hook_IDirect3DDevice9Ex_ResetEx_Detour);
    Dx9Hook_IDirect3DDevice9Ex_ResetEx = (TYPE_IDirect3DDevice9Ex_ResetEx)DetourResetEx->GetTrampoline();

#ifdef _M_IX86

    IDirect3DSwapChain9 *swapChain;
    HRESULT result;

    result = Device->GetSwapChain(0, &swapChain);

    if (result == S_OK)
    {
        virtualMethodTable = (VOID**)swapChain;
        virtualMethodTable = (VOID**)virtualMethodTable[0];

        DetourSwapChainPresent = new DetourXS((VOID*)virtualMethodTable[3], IDirect3DSwapChain9_Present_Detour);
        D3D9Hook_IDirect3DSwapChain9_Present = (TYPE_IDirect3DSwapChain9_Present)DetourSwapChainPresent->GetTrampoline();
    }

    //vmt = (VOID**) d3d9ex;
    //vmt = (VOID**) vmt[0];

    //detour = new DetourXS(vmt[16], IDirect3D9_CreateDevice_Detour);

    //Dx9Hook_IDirect3D9_CreateDevice = (TYPE_IDirect3D9_CreateDevice)detour->GetTrampoline();
#endif

    Log(L"<= ApplyDetourXsHooks()");
}


VOID DestroyDetourXsHooks()
{
    Log(L"=> DestroyDetourXsHooks()");

    DetourTestCooperativeLevel->Destroy();
    DetourReset->Destroy();
    DetourPresent->Destroy();
    DetourResetEx->Destroy();
    DetourPresentEx->Destroy();
    DetourSwapChainPresent->Destroy();

    Log(L"<= DestroyDetourXsHooks()");
}


IDirect3D9Ex *d3d9ex;


IDirect3DDevice9Ex* FindDevice(VOID)
{
    LoadLibraryW(L"d3d9.dll");

    DWORD64 pattern;

#ifdef _M_IX86
    pattern = FindPattern(L"d3d9.dll", PATT_CD3DHAL_VFTABLE_x86, MASK_CD3DHAL_VFTABLE_x86);
    pattern += 2;
    Log(L"device: 0x%X", pattern);
#else
    pattern = FindPattern64(L"d3d9.dll", PATT_CD3DHAL_VFTABLE_x64, MASK_CD3DHAL_VFTABLE_x64);
    Log(L"device: 0x%llX", pattern);
#endif

    return (IDirect3DDevice9Ex*) pattern;
}


IDirect3DDevice9Ex* BuildDevice()
{
    VOID *base = NULL;
    HRESULT result;
    D3DDISPLAYMODE d3dDisplayMode;
    D3DPRESENT_PARAMETERS presentationParameters;
    IDirect3DDevice9Ex* device;

    // Get module handle
    base = LoadLibraryW(L"d3d9.dll");

    Dx9Hook_Direct3DCreate9Ex = (TYPE_Direct3DCreate9Ex)GetProcAddress(
        (HMODULE)base,
        "Direct3DCreate9Ex"
        );

    if (Dx9Hook_Direct3DCreate9Ex == NULL)
        return NULL;

    result = Dx9Hook_Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);

    if (FAILED(result))
        return NULL;

    ZeroMemory(&presentationParameters, sizeof(D3DPRESENT_PARAMETERS));

    d3d9ex->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3dDisplayMode);

    presentationParameters.Windowed = TRUE;
    presentationParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentationParameters.BackBufferFormat = d3dDisplayMode.Format;

    result = d3d9ex->CreateDeviceEx(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        GetDesktopWindow(),
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &presentationParameters,
        NULL,
        &device
        );

    if (FAILED(result))
        return NULL;

    return device;
}


DetourXS *dinptdt;
LPDIENUMDEVICESCALLBACKA D3D9Hook_enumcallbck;


BOOL __stdcall enumCallbackDetour(const DIDEVICEINSTANCEA* instance, VOID* context)
{
    BOOL result;
    DIDEVICEINSTANCEA *lolstance;

    lolstance = (DIDEVICEINSTANCEA*) instance;
    unsigned int type = GET_DIDEVICE_TYPE(instance->dwDevType);
    unsigned int subtype = GET_DIDEVICE_SUBTYPE(instance->dwDevType);

    if (type == DI8DEVTYPE_1STPERSON && subtype == DI8DEVTYPE1STPERSON_SIXDOF)
    {
        Log(L"dwDevType: 0x%X", instance->dwDevType);
        Log(L"type: 0x%X", GET_DIDEVICE_TYPE(instance->dwDevType));
        Log(L"subtype: 0x%X", GET_DIDEVICE_SUBTYPE(instance->dwDevType));

        //lolstance->dwDevType = 0x10215; //DI8DEVTYPE_GAMEPAD | DI8DEVTYPEGAMEPAD_STANDARD
    }

    result = D3D9Hook_enumcallbck(instance, context);

    return result;
}


HRESULT __stdcall IDirectInput8_EnumDevices_Detour(
    IDirectInput8A* Device,
    DWORD dwDevType,
    LPDIENUMDEVICESCALLBACKA lpCallback,
    LPVOID pvRef,
    DWORD dwFlags
    )
{
    HRESULT result;

    D3D9Hook_enumcallbck = (LPDIENUMDEVICESCALLBACKA)lpCallback;

    result = D3D9Hook_IDirectInput8_EnumDevices(Device, dwDevType, enumCallbackDetour, pvRef, dwFlags);

    return result;
}


HRESULT __stdcall  DirectInput8Create_Detour(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN punkOuter
    )
{
    HRESULT result;

    result = D3D9Hook_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

    if (result != S_OK)
    {
        return result;
    }

    static bool ronce = FALSE;

    if (!ronce)
    {
        LPDIRECTINPUT8A di;
        DWORD dwFuncTable = (DWORD)*((DWORD*)*ppvOut);

        di = (LPDIRECTINPUT8A) (DWORD)*((DWORD*)*ppvOut);

        //Log(L"di 0x%x", di);
        //Log(L"dwFuncTable 0x%x!", dwFuncTable);

        VOID **virtualMethodTable;

        virtualMethodTable = (VOID**)/*di;*/dwFuncTable;
        virtualMethodTable = (VOID**)virtualMethodTable[0];

        ronce = TRUE;

        //DWORD oldCreateDevice = *((DWORD*)(dwFuncTable + 0x0C)); //4 * 3 (virtualMethodTable[3]) = 12 = 0x0C
        DWORD oldEnumDevice = *((DWORD*)(dwFuncTable + 0x10)); //4 * 4 (virtualMethodTable[4]) = 16 = 0x10

        Detour_DirectInput_EnumDevices = new DetourXS(/*virtualMethodTable[4]*/(VOID*)oldEnumDevice, IDirectInput8_EnumDevices_Detour);
        D3D9Hook_IDirectInput8_EnumDevices = (TYPE_IDirectInput8_EnumDevices) Detour_DirectInput_EnumDevices->GetTrampoline();
    }

    return result;
}


VOID Dx9Hook_Initialize( D3D9HOOK_PARAMS* HookParams )
{
    IDirect3DDevice9Ex *device;

    Dx9Hook_Present = HookParams->PresentCallback;
    Dx9Hook_Reset = HookParams->ResetCallback;
    Dx9Hook_CreateDevice = HookParams->CreateDeviceCallback;

    device = BuildDevice();
    
    // if fail then no hooks for you.
    if (!device)
        return;

    if (D3D9Hook_HookMethod == HOOK_METHOD_VMT)
    {
        ReplaceVirtualMethods(device);
    }
    else if (D3D9Hook_HookMethod == HOOK_METHOD_DETOUR)
    {
        ApplyDetourXsHooks(device);
    }

    //dinput hook
    DWORD base = NULL;
    DWORD func = NULL;
    base = (DWORD)GetModuleHandleW(L"dinput8.dll");

    if (!base)
    {
        return;
    }

    func = (DWORD)GetProcAddress((HMODULE)base, "DirectInput8Create");

    if (!func)
    {
        Log(L"not DirectInput8Create!");
        return;
    }

    Log(L"got DirectInput8Create...");

    dinptdt = new DetourXS((VOID*)func, DirectInput8Create_Detour);
    D3D9Hook_DirectInput8Create = (TYPE_DirectInput8Create) dinptdt->GetTrampoline();
}


VOID Dx9Hook_Destroy()
{
    DestroyDetourXsHooks();
}
