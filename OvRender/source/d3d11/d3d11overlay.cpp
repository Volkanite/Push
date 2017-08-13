#include <windows.h>
#include <D3D11.h>

#include "d3d11overlay.h"


typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;

// maps unsigned 8 bits/channel to D3DCOLOR
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
    

#include "d3d11font.h"


Dx11Font            *Dx11OvFont;
ID3D11Texture2D *BackBuffer;
ID3D11RenderTargetView  *RenderTarget;


Dx11Overlay::Dx11Overlay(
    IDXGISwapChain* SwapChain,
    OV_RENDER RenderFunction
    )
{
    ID3D11Device *device;
    
    D3D11_TEXTURE2D_DESC BackBufferDescription;

    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
    BackBuffer->GetDesc(&BackBufferDescription);

    BackBufferWidth = BackBufferDescription.Width;
    BackBufferHeight = BackBufferDescription.Height;

    SwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&device);

    device->CreateRenderTargetView(BackBuffer, NULL, &RenderTarget);

    BackBuffer->Release();

    Dx11OvFont = new Dx11Font( device );
    UserRenderFunction = RenderFunction;

    
}


Dx11Overlay::~Dx11Overlay()
{
    if (RenderTarget)
    {
        RenderTarget->Release();
        RenderTarget = 0;
    }
}


VOID Dx11Overlay::DrawText( WCHAR* Text )
{
    DrawText(Text, 0xFFFFFF00);
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
    Dx11OvFont->DrawText((FLOAT)X, (FLOAT)Y, Color, Text);
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
