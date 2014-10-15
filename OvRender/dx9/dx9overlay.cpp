#include <windows.h>
#include <d3d9.h>
//#include <sld3d9hook.h>

#include "dx9overlay.h"
#include "dx9font.h"
#include "dx9hook.h"
#include "overlay.h"



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
}


VOID
Dx9OvRender( IDirect3DDevice9* Device )
{
    if (Dx9OvFont == NULL)
    {
        Dx9OvFont = new Dx9Font(Device);

        Dx9OvFont->InitDeviceObjects();
        Dx9OvFont->RestoreDeviceObjects();
        
        Dx9OvDevice = Device;
    }

    OvDx9Overlay->Render();
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
Dx9Overlay::DrawPrimitiveUP( 
    OVPRIMITIVETYPE PrimitiveType, 
    UINT PrimitiveCount, 
    const void *VertexData, 
    UINT VertexSize 
    )
{
    Dx9OvDevice->DrawPrimitiveUP( 
        (D3DPRIMITIVETYPE)PrimitiveType, 
        PrimitiveCount, 
        VertexData, 
        VertexSize 
        );
}


VOID
Dx9Overlay::Begin()
{

}


VOID
Dx9Overlay::End()
{

}

