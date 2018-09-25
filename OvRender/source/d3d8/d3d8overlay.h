#include <OvRender.h>


class Dx8Overlay : public OvOverlay
{
public:
    Dx8Overlay( OV_RENDER RenderFunction );
    ~Dx8Overlay();

    // Standard functions
    VOID DrawText( WCHAR* Text );
    VOID DrawText( WCHAR* Text, DWORD Color );
    VOID DrawText( WCHAR* Text, int X, int Y, DWORD Color );
    VOID Begin();
    VOID End();
    VOID* GetDevice();
};
