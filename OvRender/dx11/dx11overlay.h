#include <OvRender.h>


class Dx11Overlay : public OvOverlay
{
public:
    Dx11Overlay( ID3D11Device* Device, OV_RENDER RenderFunction );

    // Standard functions
    VOID DrawText( WCHAR* Text );
    VOID DrawText( WCHAR* Text, DWORD Color );
    VOID DrawText( WCHAR* Text, int X, int Y, DWORD Color );
    VOID DrawPrimitiveUP( OVPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *VertexData, UINT VertexSize );
    VOID Begin();
    VOID End();
};

