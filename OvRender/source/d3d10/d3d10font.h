#include "font.h"


class Dx10Font : public Font
{
public:
    Dx10Font( ID3D10Device *device );
    VOID Begin();
    VOID End();
    VOID DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, WCHAR* Text );
    VOID AddString( WCHAR *text, BOOLEAN overload);
    VOID BeginBatch();
    VOID DrawBatch( UINT startSpriteIndex, UINT spriteCount );
    VOID EndBatch();
    BOOLEAN InitD3D10Sprite( );
    VOID Draw( RECT* destinationRect, RECT* sourceRect, XMCOLOR color );

    // Standard functions
    DWORD GetMaxTextureWidth();
    HRESULT CreateTexture();
    VOID LockTexture( D3DLOCKED_RECT *pLockedRect );
    VOID UnlockTexture();
};
