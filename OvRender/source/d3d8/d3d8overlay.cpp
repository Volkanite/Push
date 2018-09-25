#include <windows.h>
#include <d3d8.h>

#include "d3d8font.h"
#include "d3d8overlay.h"
#include "d3d8hook.h"


Dx8Font* D3D8Font;
extern Dx8Overlay* D3D8Overlay;


VOID IDirect3DDevice8_PresentCallback( 
    IDirect3DDevice8 *Device 
    )
{
    if (D3D8Font == NULL)
    {
        D3D8Font = new Dx8Font(Device);
        D3D8Font->RestoreDeviceObjects();

        GraphicsApi = API_D3D8;
    }

    D3D8Overlay->Render();
}


VOID IDirect3DDevice8_ResetCallback()
{
    if (D3D8Font)
    {
        D3D8Font->InvalidateDeviceObjects();
        D3D8Font->DeleteDeviceObjects();
    }

    D3D8Font = NULL;
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


Dx8Overlay::~Dx8Overlay()
{
    Dx8Hook_Destroy();

    if (D3D8Font)
        D3D8Font->~Dx8Font();
}


VOID
Dx8Overlay::DrawText( WCHAR* Text )
{
    DrawText(Text, FALSE);
}


VOID Dx8Overlay::DrawText( 
    WCHAR* Text, 
    DWORD Color 
    )
{
    DrawText(Text, 20, Line, Color);

    Line += 15;
}


VOID Dx8Overlay::DrawText( 
    WCHAR* Text, 
    int X, 
    int Y, 
    DWORD Color 
    )
{
    D3D8Font->DrawText(
        (FLOAT)X, 
        (FLOAT)Y, 
        Color, 
        Text, 
        NULL
        );
}


VOID
Dx8Overlay::Begin()
{

}


VOID
Dx8Overlay::End()
{

}


VOID*
Dx8Overlay::GetDevice()
{
    return NULL;
}

