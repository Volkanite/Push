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


VOID UpdatePresentationParameters( D3DPRESENT_PARAMETERS* PresentationParameters )
{
    // Force vsync?
    
    if (OvDx9Overlay->VsyncOverrideMode == VSYNC_FORCE_ON)
    {
        PresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    }
    else if (OvDx9Overlay->VsyncOverrideMode == VSYNC_FORCE_OFF)
    {
        PresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }
}


VOID Dx9Overlay_CreateDevice( D3DPRESENT_PARAMETERS* PresentationParameters )
{
    UpdatePresentationParameters(PresentationParameters);
}


VOID Dx9Overlay_Reset( D3DPRESENT_PARAMETERS* PresentationParameters )
{
    if (Dx9OvFont)
    {
        Dx9OvFont->InvalidateDeviceObjects();
        Dx9OvFont->DeleteDeviceObjects();
    }

    Dx9OvFont = NULL;

    UpdatePresentationParameters(PresentationParameters);
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
Dx9Overlay_Present( IDirect3DDevice9* Device )
{
    Dx9OvRender(Device);
}


Dx9Overlay::Dx9Overlay(
    OV_RENDER RenderFunction
    )
{
    D3D9HOOK_PARAMS hookParams;

    hookParams.PresentCallback = Dx9Overlay_Present;
    hookParams.ResetCallback = Dx9Overlay_Reset;
    hookParams.CreateDeviceCallback = Dx9Overlay_CreateDevice;

    Dx9Hook_Initialize(&hookParams);

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