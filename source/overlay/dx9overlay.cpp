#include <windows.h>
#include <d3d9.h>
//#include <sld3d9hook.h>

#include "dx9overlay.h"
#include "dx9font.h"
#include "dx9hook.h"
#include <OvRender.h>



BOOLEAN HkInitialized = FALSE;
Dx9Font* Dx9OvFont;
extern Dx9Overlay* OvDx9Overlay;
IDirect3DDevice9* Dx9OvDevice;


VOID
IDirect3D_CreateDeviceCallback(
    D3DPRESENT_PARAMETERS* PresentationParameters
    )
{

}


VOID
IDirect3DDevice9_ResetCallback(
    D3DPRESENT_PARAMETERS* PresentationParameters
    )
{
    if (Dx9OvFont)
    {
        Dx9OvFont->InvalidateDeviceObjects();
        Dx9OvFont->DeleteDeviceObjects();
    }

    Dx9OvFont = NULL;

    // Force vsync?
    if (OvDx9Overlay->ForceVsync)
        PresentationParameters->PresentationInterval = 1;
}


VOID
Dx9OvRender( IDirect3DDevice9* Device )
{
    IDirect3DSurface9 *renderTarget = NULL;
    IDirect3DSurface9 *backBuffer = NULL;
    D3DVIEWPORT9 viewport;

    if (Dx9OvFont == NULL)
    {
        Dx9OvFont = new Dx9Font(Device);

        Dx9OvFont->InitDeviceObjects();
        Dx9OvFont->RestoreDeviceObjects();
        
        Dx9OvDevice = Device;
    }

    // Backup render target.
    
    Device->GetRenderTarget( 0, &renderTarget );
    Device->GetViewport(&viewport);

    // Set backbuffer as new render target.
    
    Device->GetBackBuffer( 
        0, 
        0, 
        D3DBACKBUFFER_TYPE_MONO, 
        &backBuffer 
        );

    Device->SetRenderTarget( 0, backBuffer );
    
    // Render our stuff.
    OvDx9Overlay->Render();

    // Restore render target.

    Device->SetRenderTarget( 0, renderTarget );
    Device->SetViewport(&viewport);

    // Cleanup.

    backBuffer->Release();
    renderTarget->Release();
}


VOID
IDirect3DDevice9_PresentCallback(
    IDirect3DDevice9* Device
    )
{
    Dx9OvRender(Device);
}


VOID
IDirect3DSwapChain9_PresentCallback(
    IDirect3DDevice9* Device
    )
{
    Dx9OvRender(Device);
}


Dx9Overlay::Dx9Overlay(
    OV_RENDER RenderFunction
    )
{
    SlHookD3D9(
        IDirect3DDevice9_PresentCallback,
        IDirect3DSwapChain9_PresentCallback,
        IDirect3DDevice9_ResetCallback,
        IDirect3D_CreateDeviceCallback
        );

    UserRenderFunction = RenderFunction;
}


VOID
Dx9Overlay::DrawText( WCHAR* Text )
{   
    DrawText(Text, 0xFFFFFF00);
}


VOID
Dx9Overlay::DrawText( WCHAR* Text, DWORD Color )
{
    DrawText(Text, 20, Line, Color);

    Line += 15;
}


VOID 
Dx9Overlay::DrawText( WCHAR* Text, int X, int Y, DWORD Color )
{
    Dx9OvFont->DrawText(X, Y, Color, Text, NULL);
}


VOID
Dx9Overlay::Begin()
{

}


VOID
Dx9Overlay::End()
{

}


VOID*
Dx9Overlay::GetDevice()
{
	return Dx9OvDevice;
}