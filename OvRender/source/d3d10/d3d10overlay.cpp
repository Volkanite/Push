#include <windows.h>
#include <d3dx10.h>


typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;
#include "d3d10font.h"
#include "d3d10overlay.h"

// maps unsigned 8 bits/channel to D3DCOLOR
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))

Dx10Font* Dx10OvFont;


Dx10Overlay::Dx10Overlay(
    ID3D10Device* Device,
    OV_RENDER RenderFunction
    )
{
    Dx10OvFont = new Dx10Font( Device );
    UserRenderFunction = RenderFunction;
}


VOID
Dx10Overlay::DrawText( WCHAR* Text )
{
    DrawText(Text, FALSE);
}


VOID
Dx10Overlay::DrawText( WCHAR* Text, DWORD Color )
{
    DrawText(Text, 20, Line, Color);

    Line += 15;
}


VOID Dx10Overlay::DrawText( WCHAR* Text, int X, int Y, DWORD Color )
{
    Dx10OvFont->DrawText((FLOAT)X, (FLOAT)Y, Color, Text);
}


VOID
Dx10Overlay::Begin()
{
    Dx10OvFont->Begin();
}


VOID
Dx10Overlay::End()
{
    Dx10OvFont->End();
}


VOID*
Dx10Overlay::GetDevice()
{
    return NULL;
}