#include "font.h"


class Dx11Font : public Font
{
public:
    Dx11Font( ID3D11Device *device );
    VOID Begin();
    VOID End();
    VOID BeginBatch( ID3D11ShaderResourceView* texSRV );
    VOID DrawBatch( UINT startSpriteIndex, UINT spriteCount );
    VOID EndBatch( );
    VOID DrawString();
    BOOLEAN InitD3D11Sprite( );

    // Standard functions
    DWORD GetMaxTextureWidth();
    HRESULT MapTexture(D3DLOCKED_RECT *pLockedRect);
    HRESULT UnmapTexture();
};