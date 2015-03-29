// Font creation flags
#define D3DFONT_BOLD        0x0001
#define D3DFONT_ITALIC      0x0002
#define D3DFONT_ZENABLE     0x0004

// Font rendering flags
#define D3DFONT_CENTERED    0x0001
#define D3DFONT_TWOSIDED    0x0002
#define D3DFONT_FILTERED    0x0004
#define D3DFONT_RIGHT       0x0008 // non standard feature
#define D3DFONT_SHADOW      0x0010 // non standard feature

#define FONT_SIZE 10
#define PIXEL_DEPTH 32
#define NUMBER_OF_CHARACTERS 177


#include <xmmintrin.h>
#include <emmintrin.h>
#include <math.h>
#include <slxnamath.h>


typedef struct _Sprite
{
    RECT SrcRect;
    RECT DestRect;
    XMCOLOR        Color;
    FLOAT        Z;
    FLOAT        Angle;
    FLOAT        Scale;

}Sprite;

typedef struct _SpriteVertex
{
    float x;
    float y;
    float z;
    XMFLOAT2 Tex;
    XMCOLOR     Color;

}SpriteVertex;


class Font{
public:
    WCHAR   m_strFontName[80];            // Font properties
    DWORD   m_dwFontHeight;
    DWORD   m_dwFontFlags;
    DWORD   m_dwTexWidth;
    DWORD   m_dwTexHeight;
    DWORD   m_dwSpacing;
    FLOAT   m_fTextScale;
    FLOAT   m_fTexCoords[ (NUMBER_OF_CHARACTERS+1) - 32 ][4];
    FLOAT   ScreenWidth;
    FLOAT   ScreenHeight;
    UINT  NumberOfSprites;
    UINT  SpriteListSize;
    Sprite* Sprites;
    VOID*   HeapHandle;

    Font();
    HRESULT GetTextExtent( TCHAR* strText, SIZE* pSize );
    HRESULT InitDeviceObjects();
    VOID AddSprite( Sprite *sprite );
    VOID BuildSpriteQuad( Sprite *sprite, SpriteVertex v[ 4 ] );

    virtual DWORD GetMaxTextureWidth() = 0;
    virtual HRESULT CreateTexture() = 0;
    virtual VOID LockTexture( D3DLOCKED_RECT *pLockedRect ) = 0;
    virtual VOID UnlockTexture() = 0;
};
