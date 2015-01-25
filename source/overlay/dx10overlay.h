#include <OvRender.h>


class Dx10Overlay : public OvOverlay
{
public:
    Dx10Overlay( ID3D10Device* Device, OV_RENDER RenderFunction );

    // Standard functions
    VOID DrawText( WCHAR* Text );
    VOID DrawText( WCHAR* Text, DWORD Color );
    VOID DrawText( WCHAR* Text, int X, int Y, DWORD Color );
    VOID Begin();
    VOID End();
};
