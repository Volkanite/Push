#include <Windows.h>
#include <DXGI.h>
#include <D3D10.h>
#include <hexus.h>

#include "dxgihook.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d10.lib")


typedef HRESULT (__stdcall* TYPE_IDXGISwapChain_Present) (
    IDXGISwapChain *SwapChain,
    UINT SyncInterval,
    UINT Flags
    );

typedef HRESULT(__stdcall* TYPE_IDXGISwapChain_ResizeBuffers) (
    IDXGISwapChain *SwapChain,
    UINT BufferCount,
    UINT Width,
    UINT Height,
    DXGI_FORMAT NewFormat,
    UINT SwapChainFlags
    );

TYPE_IDXGISwapChain_Present         HkIDXGISwapChain_Present;
TYPE_IDXGISwapChain_ResizeBuffers   HkIDXGISwapChain_ResizeBuffers;

typedef VOID (*HK_IDXGISWAPCHAIN_CALLBACK)(
IDXGISwapChain* SwapChain
);
HK_IDXGISWAPCHAIN_CALLBACK HkIDXGISwapChain_PresentCallback;
HK_IDXGISWAPCHAIN_CALLBACK HkIDXGISwapChain_ResizeBuffersCallback;


VOID OvLog(const wchar_t* Format, ...);
DWORD FindPattern(WCHAR* Module, char pattern[], char mask[]);
DWORD64 FindPattern64(WCHAR* Module, char pattern[], char mask[]);
void ReplaceVirtualMethod(void **VTable, int Function, void *Detour);


HRESULT __stdcall IDXGISwapChain_PresentHook(
    IDXGISwapChain* SwapChain,
    UINT SyncInterval,
    UINT Flags
    )
{
    static BOOLEAN init = FALSE;

    if (!init)
    {
        init = TRUE;

        OvLog(L"Hook called @ IDXGISwapChain_Present");
    }

    if (HkIDXGISwapChain_PresentCallback)
    {
        HkIDXGISwapChain_PresentCallback(SwapChain);
    }

    return HkIDXGISwapChain_Present(SwapChain, SyncInterval, Flags);
}


HRESULT __stdcall IDXGISwapChain_ResizeBuffersHook( 
    IDXGISwapChain *SwapChain, 
    UINT BufferCount, 
    UINT Width, 
    UINT Height, 
    DXGI_FORMAT NewFormat, 
    UINT SwapChainFlags 
    )
{
    if (HkIDXGISwapChain_ResizeBuffersCallback)
    {
        HkIDXGISwapChain_ResizeBuffersCallback(SwapChain);
    }

    return HkIDXGISwapChain_ResizeBuffers(
        SwapChain,
        BufferCount,
        Width,
        Height,
        NewFormat,
        SwapChainFlags
        );
}


LRESULT 
CALLBACK 
FakeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


IDXGISwapChain* BuildSwapChain()
{
    IDXGIFactory* factory;
    IDXGIAdapter *pAdapter;
    IDXGISwapChain* swapChain;
    ID3D10Device *pDevice;
    UINT CreateFlags = 0;
    DXGI_MODE_DESC requestedMode;
    DXGI_SWAP_CHAIN_DESC scDesc;
    HWND windowHandle;
    HRESULT hr;

    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    factory->EnumAdapters(0, &pAdapter);

    D3D10CreateDevice(
        pAdapter,
        D3D10_DRIVER_TYPE_HARDWARE,
        NULL,
        CreateFlags,
        D3D10_SDK_VERSION,
        &pDevice
        );

    requestedMode.Width = 500;
    requestedMode.Height = 500;
    requestedMode.RefreshRate.Numerator = 0;
    requestedMode.RefreshRate.Denominator = 0;
    requestedMode.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    requestedMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    requestedMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    // Create fake window

    WNDCLASSEX windowClass = { 0 };

    windowClass.lpfnWndProc = FakeWndProc;
    windowClass.lpszClassName = L"JustGimmeADamnWindow";
    windowClass.cbSize = sizeof(WNDCLASSEX);

    ATOM classy = RegisterClassExW(&windowClass);

    if (!classy)
    {
        OvLog(L"RegisterClassExW failed with 0x%i", GetLastError());
        return NULL;
    }

    windowHandle = CreateWindowExW(
        0,
        L"JustGimmeADamnWindow",
        L"HwBtYouJustGimmeMyDamnWindow?",
        WS_SYSMENU,
        CW_USEDEFAULT,
        NULL,
        300,
        300,
        0, 0, 0, 0
        );

    if (!windowHandle)
    {
        OvLog(L"CreateWindowExW failed with 0x%i", GetLastError());
        return NULL;
    }

    // Now create the thing

    scDesc.BufferDesc = requestedMode;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.OutputWindow = windowHandle;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    hr = factory->CreateSwapChain(pDevice, &scDesc, &swapChain);

    if (FAILED(hr))
    {
        OvLog(L"IDXGIFactory::CreateSwapChain failed! hr=0x%X",hr);
        return NULL;
    }

    // Destroy window
    DestroyWindow(windowHandle);
    UnregisterClassW(L"JustGimmeADamnWindow", GetModuleHandleW(NULL));

    return swapChain;
}


DETOUR_PROPERTIES DetourDXGIPresent;
DETOUR_PROPERTIES DetourDXGIResizeBuffers;


VOID DxgiHook_Initialize( IDXGISWAPCHAIN_HOOK* HookParameters )
{
    IDXGISwapChain* swapChain;
    VOID **vmt;

    HkIDXGISwapChain_PresentCallback = (HK_IDXGISWAPCHAIN_CALLBACK) HookParameters->PresentCallback;
    HkIDXGISwapChain_ResizeBuffersCallback = (HK_IDXGISWAPCHAIN_CALLBACK) HookParameters->ResizeBuffersCallback;

	swapChain = BuildSwapChain();

    if (!swapChain)
        return;

    vmt = (VOID**) swapChain;
    vmt = (VOID**) vmt[0];

    if (!HkIDXGISwapChain_Present)
    {
        DetourCreate(vmt[8], IDXGISwapChain_PresentHook, &DetourDXGIPresent);
        HkIDXGISwapChain_Present = (TYPE_IDXGISwapChain_Present)DetourDXGIPresent.Trampoline;
        
        DetourCreate(vmt[13], IDXGISwapChain_ResizeBuffersHook, &DetourDXGIResizeBuffers);
        HkIDXGISwapChain_ResizeBuffers = (TYPE_IDXGISwapChain_ResizeBuffers)DetourDXGIResizeBuffers.Trampoline;
    }

	swapChain->Release();
}


VOID DxgiHook_Destroy()
{
    if (DetourDXGIPresent.Trampoline)
        DetourDestroy(&DetourDXGIPresent);

    if (DetourDXGIResizeBuffers.Trampoline)
        DetourDestroy(&DetourDXGIResizeBuffers);
}