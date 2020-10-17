#include <Windows.h>
#include <DXGI.h>
#include <stdio.h>
#include <D3D11.h>
#include <D3D10.h>

#include <OvRender.h>
#include "dxgioverlay.h"
#include <d3d11overlay.h>
#include <d3d10overlay.h>
#include "dxgihook.h"


Dx11Overlay* D3D11Overlay;
Dx10Overlay* DxgiOvDx10Overlay;
extern DxgiOverlay* DXGIOverlay;
extern HWND OvWindowHandle;


typedef HRESULT (WINAPI* TYPE_IDXGIFactory_CreateSwapChain) (
    VOID *factory,
    IUnknown *pDevice,
                                             DXGI_SWAP_CHAIN_DESC *pDesc,
                                             IDXGISwapChain **ppSwapChain);


TYPE_IDXGIFactory_CreateSwapChain   DxgiOvIDXGIFactory_CreateSwapChain = 0;
BOOLEAN DxgiOverlayInitialized;


VOID DxgiOvInit( IDXGISwapChain* SwapChain )
{
    ID3D11Device *device11;
    ID3D10Device *device10;

    if (SUCCEEDED(SwapChain->GetDevice(__uuidof(ID3D11Device), (void **) &device11)))
    {
        D3D11Overlay = new Dx11Overlay(SwapChain, DXGIOverlay->UserRenderFunction);
        GraphicsApi = API_D3D11;
        D3D11Overlay->FontProperties = DXGIOverlay->FontProperties;
		OvLog(L"DxgiOvInit D3D11");
    }
    else if (SUCCEEDED(SwapChain->GetDevice(__uuidof(ID3D10Device), (void **) &device10)))
    {
        DxgiOvDx10Overlay = new Dx10Overlay(device10, DXGIOverlay->UserRenderFunction);
        GraphicsApi = API_D3D10;
		OvLog(L"DxgiOvInit D3D10");
    }

    DxgiOverlayInitialized = TRUE;
}


VOID IDXGISwapChain_PresentCallback( IDXGISwapChain* SwapChain )
{
    if (!DxgiOverlayInitialized)
        DxgiOvInit( SwapChain );

    if (D3D11Overlay)
        D3D11Overlay->Render();

    else if (DxgiOvDx10Overlay)
        DxgiOvDx10Overlay->Render();
}


VOID IDXGISwapChain_ResizeBuffersCallback( IDXGISwapChain* SwapChain )
{
    if (D3D11Overlay)
    {
        D3D11Overlay->~Dx11Overlay();
        D3D11Overlay = NULL;
    }

    DxgiOverlayInitialized = FALSE;
}


DxgiOverlay::DxgiOverlay( OV_RENDER RenderFunction )
{
    IDXGISWAPCHAIN_HOOK hookParams;

    UserRenderFunction = RenderFunction;

    hookParams.PresentCallback = IDXGISwapChain_PresentCallback;
    hookParams.ResizeBuffersCallback = IDXGISwapChain_ResizeBuffersCallback;
	hookParams.WindowHandle = OvWindowHandle;

    DxgiHook_Initialize(&hookParams);
}


DxgiOverlay::~DxgiOverlay()
{
    DxgiHook_Destroy();
}


VOID
DxgiOverlay::DrawText( WCHAR* Text )
{

}


VOID
DxgiOverlay::DrawText( WCHAR* Text, DWORD Color )
{

}


VOID
DxgiOverlay::DrawText( WCHAR* Text, int X, int Y, DWORD Color )
{

}


VOID
DxgiOverlay::Begin()
{

}


VOID
DxgiOverlay::End()
{

}


VOID*
DxgiOverlay::GetDevice()
{
    return NULL;
}
