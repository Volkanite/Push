#include <windows.h>
#include <d3d8.h>

#include "dx8font.h"
#include "dx8overlay.h"
#include "dx8hooks.h"


Dx8Font* Dx8OvFont;
extern Dx8Overlay* OvDx8Overlay;


VOID
IDirect3DDevice8_PresentCallback( IDirect3DDevice8 *Device )
{
    if (Dx8OvFont == NULL)
    {
        Dx8OvFont = new Dx8Font(Device);

        Dx8OvFont->InitDeviceObjects();
        Dx8OvFont->RestoreDeviceObjects();
    }

    OvDx8Overlay->Render();
}


VOID
IDirect3DDevice8_ResetCallback()
{
    if (Dx8OvFont)
    {
        Dx8OvFont->InvalidateDeviceObjects();
        Dx8OvFont->DeleteDeviceObjects();
    }

    Dx8OvFont = NULL;
}


Dx8Overlay::Dx8Overlay(
    OV_RENDER RenderFunction
    )
{
    HookD3D8( 
        IDirect3DDevice8_PresentCallback, 
        IDirect3DDevice8_ResetCallback 
        );

    UserRenderFunction = RenderFunction;
}


VOID
Dx8Overlay::DrawText( WCHAR* Text )
{
    DrawText(Text, FALSE);
}


VOID
Dx8Overlay::DrawText( WCHAR* Text, DWORD Color )
{
    DrawText(Text, 20, Line, Color);

    Line += 15;
}


VOID 
Dx8Overlay::DrawText( WCHAR* Text, int X, int Y, DWORD Color )
{
    Dx8OvFont->DrawText(X, Y, Color, Text, NULL);
}


VOID
Dx8Overlay::DrawPrimitiveUP( 
    OVPRIMITIVETYPE PrimitiveType, 
    UINT PrimitiveCount, 
    const void *VertexData, 
    UINT VertexSize 
    )
{

}


VOID
Dx8Overlay::Begin()
{

}


VOID
Dx8Overlay::End()
{

}


