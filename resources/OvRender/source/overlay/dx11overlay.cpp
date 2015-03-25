#include <windows.h>
#include <D3D11.h>

#include "dx11overlay.h"


typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;

// maps unsigned 8 bits/channel to D3DCOLOR
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
    

#include "dx11font.h"


Dx11Font* Dx11OvFont;


Dx11Overlay::Dx11Overlay(
    ID3D11Device* Device,
    OV_RENDER RenderFunction
    )
{
    Dx11OvFont = new Dx11Font( Device );
    UserRenderFunction = RenderFunction;
}


VOID
Dx11Overlay::DrawText( WCHAR* Text )
{
    DrawText( Text, FALSE );
}


VOID
Dx11Overlay::DrawText( WCHAR* Text, DWORD Color )
{
    DrawText(Text, 20, Line, Color);

    Line += 15;
}


VOID 
Dx11Overlay::DrawText( WCHAR* Text, int X, int Y, DWORD Color )
{
    Dx11OvFont->DrawText(X, Y, Color, Text);
}


VOID
Dx11Overlay::Begin()
{
    Dx11OvFont->Begin();
}


VOID
Dx11Overlay::End()
{
    Dx11OvFont->End();
}


VOID*
Dx11Overlay::GetDevice()
{
	return NULL;
}
