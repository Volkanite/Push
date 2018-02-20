#include <font.h>


class Dx10Font : public Font
{
public:
    Dx10Font( ID3D10Device *device );
    VOID Begin();
    VOID End();
    VOID BeginBatch();
    VOID DrawBatch( UINT startSpriteIndex, UINT spriteCount );
    VOID EndBatch();
    BOOLEAN InitD3D10Sprite( );

    // Standard functions
    DWORD GetMaxTextureWidth();
    HRESULT MapTexture(D3DLOCKED_RECT *pLockedRect);
    HRESULT UnmapTexture();
};
