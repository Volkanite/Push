#include <font.h>


class Dx11Font : public Font
{
public:
    Dx11Font( ID3D11Device *device );
    VOID Begin();
    VOID End();
    VOID DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, WCHAR* Text );
    VOID AddString( WCHAR *text, DWORD Color );
    VOID BeginBatch( ID3D11ShaderResourceView* texSRV );
    VOID DrawBatch( UINT startSpriteIndex, UINT spriteCount );
    VOID EndBatch( );
    VOID DrawString();
    BOOLEAN InitD3D11Sprite( );
    VOID Draw( RECT* destinationRect, RECT* sourceRect, XMCOLOR color );

    // Standard functions
    DWORD GetMaxTextureWidth();
    HRESULT CreateTexture();
    VOID LockTexture( D3DLOCKED_RECT *pLockedRect );
    VOID UnlockTexture();
};