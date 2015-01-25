#include <Windows.h>
#include <DXGI.h>
#include <D3D10.h>
#include <sldetours.h>
#include "dxgihooks.h"


typedef HRESULT (WINAPI* TYPE_IDXGISwapChain_Present) (
    IDXGISwapChain *pSwapChain,
    UINT SyncInterval,
    UINT Flags
    );



TYPE_IDXGISwapChain_Present HkIDXGISwapChain_Present;
HK_IDXGISWAPCHAIN_PRESENT_CALLBACK HkIDXGISwapChain_PresentCallback;


HRESULT
WINAPI
IDXGISwapChain_PresentHook(
    IDXGISwapChain* SwapChain,
    UINT SyncInterval,
    UINT Flags
    )
{
    if (HkIDXGISwapChain_PresentCallback)
        HkIDXGISwapChain_PresentCallback( SwapChain );

    return HkIDXGISwapChain_Present(
            SwapChain,
            SyncInterval,
            Flags
            );
}


LRESULT 
CALLBACK 
FakeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


VOID 
HookDxgi( HK_IDXGISWAPCHAIN_PRESENT_CALLBACK IDXGISwapChain_PresentCallback )
{
    IDXGIFactory* factory;
    IDXGIAdapter *pAdapter;
    IDXGISwapChain* pSwapChain;
    ID3D10Device *pDevice;
    DXGI_MODE_DESC requestedMode; 
    DXGI_SWAP_CHAIN_DESC scDesc;
    UINT CreateFlags = 0;
    HWND windowHandle;
    HRESULT hr;

    HkIDXGISwapChain_PresentCallback = IDXGISwapChain_PresentCallback;

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
    
    requestedMode.Width                   = 500; 
    requestedMode.Height                  = 500; 
    requestedMode.RefreshRate.Numerator   = 0; 
    requestedMode.RefreshRate.Denominator = 0; 
    requestedMode.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; 
    requestedMode.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; 
    requestedMode.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;

    // Create fake window
    
    WNDCLASSEX windowClass = { 0 };

    windowClass.lpfnWndProc = FakeWndProc;
    windowClass.lpszClassName = L"JustGimmeADamnWindow";
    windowClass.cbSize = sizeof(WNDCLASSEX);

    RegisterClassExW(&windowClass);

    windowHandle = CreateWindowExW(
                    0,
                    L"JustGimmeADamnWindow",
                    L"HwBtYouJustGimmeMyDamnWindow?",
                    WS_SYSMENU,
                    CW_USEDEFAULT,
                    NULL,
                    300,
                    300,
                    0,0,0,0
                    );

    // Now create the thing
    
    scDesc.BufferDesc         = requestedMode; 
    scDesc.SampleDesc.Count   = 1; 
    scDesc.SampleDesc.Quality = 0; 
    scDesc.BufferUsage        = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT; 
    scDesc.BufferCount        = 2; 
    scDesc.OutputWindow       = windowHandle;
    scDesc.Windowed           = TRUE; 
    scDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD; 
    scDesc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    hr = factory->CreateSwapChain(pDevice, &scDesc, &pSwapChain);

    if (FAILED(hr))
    {
        OutputDebugStringW(L"[OVRENDER] IDXGIFactory::CreateSwapChain failed!");
        return;
    }

    if (pSwapChain)
    {
        VOID **vmt;
        SlHookManager hookManager;

        vmt = (VOID**) pSwapChain;
        vmt = (VOID**) vmt[0];

        if (!HkIDXGISwapChain_Present)
        {
            HkIDXGISwapChain_Present = (TYPE_IDXGISwapChain_Present) hookManager.DetourFunction(
                                                (BYTE*)vmt[8],
                                                (BYTE*)IDXGISwapChain_PresentHook
                                                );
        }
    }
}