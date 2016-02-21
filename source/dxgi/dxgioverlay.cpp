#include <Windows.h>
#include <DXGI.h>
#include <stdio.h>
#include <D3D11.h>
#include <D3D10.h>

#include <OvRender.h>
#include "dxgioverlay.h"
#include "d3d11overlay.h"
#include "d3d10overlay.h"
#include "dxgihook.h"


Dx11Overlay* DxgiOvDx11Overlay;
Dx10Overlay* DxgiOvDx10Overlay;
extern DxgiOverlay* OvDxgiOverlay;




typedef HRESULT (WINAPI* TYPE_IDXGIFactory_CreateSwapChain) (
    VOID *factory,
    IUnknown *pDevice,
                                             DXGI_SWAP_CHAIN_DESC *pDesc,
                                             IDXGISwapChain **ppSwapChain);



TYPE_IDXGIFactory_CreateSwapChain   DxgiOvIDXGIFactory_CreateSwapChain = 0;
BOOLEAN DxgiOvInitialized;


VOID
DxgiOvInit( IDXGISwapChain* SwapChain )
{
    ID3D11Device *device11;
    ID3D10Device *device10;

    if (SUCCEEDED(SwapChain->GetDevice(__uuidof(ID3D11Device), (void **) &device11)))
    {
        DxgiOvDx11Overlay = new Dx11Overlay( device11, OvDxgiOverlay->UserRenderFunction );
    }
    else if (SUCCEEDED(SwapChain->GetDevice(__uuidof(ID3D10Device), (void **) &device10)))
    {
        DxgiOvDx10Overlay = new Dx10Overlay( device10, OvDxgiOverlay->UserRenderFunction );
    }

    DxgiOvInitialized = TRUE;
}


VOID
IDXGISwapChain_PresentCallback( 
    IDXGISwapChain* SwapChain 
    )
{
    if (!DxgiOvInitialized)
        DxgiOvInit( SwapChain );

    if (DxgiOvDx11Overlay)
        DxgiOvDx11Overlay->Render();

    else if (DxgiOvDx10Overlay)
        DxgiOvDx10Overlay->Render();
}


DxgiOverlay::DxgiOverlay( OV_RENDER RenderFunction )
{
    UserRenderFunction = RenderFunction;

    HookDxgi( IDXGISwapChain_PresentCallback );
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
