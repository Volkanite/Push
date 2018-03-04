#include <OvRender.h>


class DxgiOverlay : public OvOverlay
{
public:
    DxgiOverlay( OV_RENDER RenderFunction );
    ~DxgiOverlay();

    // Standard functions
    VOID DrawText( WCHAR* Text );
    VOID DrawText( WCHAR* Text, DWORD Color );
    VOID DrawText( WCHAR* Text, int X, int Y, DWORD Color );
    VOID Begin();
    VOID End();
    VOID* GetDevice();
};
