#include <windows.h>
#include <tchar.h>





/* Structure for LockRect */
typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;


#include "font.h"


Font::Font()
{
    WCHAR strFontName[] = L"Verdana";

    _tcsncpy(
        m_strFontName,
        strFontName,
        sizeof(m_strFontName) / sizeof(TCHAR)
        );

    m_strFontName[sizeof(m_strFontName) / sizeof(TCHAR) - 1] = _T('\0');
    m_dwFontHeight         = FONT_SIZE;
    m_dwFontFlags          = D3DFONT_BOLD;
    m_dwSpacing            = 0;
    ScreenWidth     = 0.0f ;
    ScreenHeight    = 0.0f ;
}


HRESULT
Font::InitDeviceObjects()
{
    HRESULT hr;
    DWORD textureWidthMax;

    // Establish the font and texture size
    m_fTextScale  = 1.0f; // Draw fonts into texture without scaling

    // Large fonts need larger textures
    if( m_dwFontHeight > 60 )
        m_dwTexWidth = m_dwTexHeight = 2048;
    else if( m_dwFontHeight > 30 )
        m_dwTexWidth = m_dwTexHeight = 1024;
    else if( m_dwFontHeight > 15 )
        m_dwTexWidth = m_dwTexHeight = 512;
    else
        m_dwTexWidth  = m_dwTexHeight = 256;

    // If requested texture is too big, use a smaller texture and smaller font,
    // and scale up when rendering.
    textureWidthMax = GetMaxTextureWidth();

    if( m_dwTexWidth > textureWidthMax )
    {
        m_fTextScale = (FLOAT)textureWidthMax / (FLOAT)m_dwTexWidth;
        m_dwTexWidth = m_dwTexHeight = textureWidthMax;
    }

    // Create a new texture for the font
    hr = CreateTexture();

    if( FAILED(hr) )
        return hr;

    // Prepare to create a bitmap
    DWORD*      pBitmapBits;
    BITMAPINFO bmi;
    ZeroMemory( &bmi.bmiHeader,  sizeof(BITMAPINFOHEADER) );
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       =  (int)m_dwTexWidth;
    bmi.bmiHeader.biHeight      = -(int)m_dwTexHeight;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biBitCount    = 32;

    // Create a DC and a bitmap for the font
    HDC     hDC       = CreateCompatibleDC( NULL );
    HBITMAP hbmBitmap = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS,
                                          (VOID**)&pBitmapBits, NULL, 0 );
    SetMapMode( hDC, MM_TEXT );

    // Create a font.  By specifying ANTIALIASED_QUALITY, we might get an
    // antialiased font, but this is not guaranteed.
    INT nHeight    = -MulDiv( m_dwFontHeight,
        (INT)(GetDeviceCaps(hDC, LOGPIXELSY) * m_fTextScale), 72 );
    DWORD dwBold   = (m_dwFontFlags&D3DFONT_BOLD)   ? FW_BOLD : FW_NORMAL;
    DWORD dwItalic = (m_dwFontFlags&D3DFONT_ITALIC) ? TRUE    : FALSE;
    HFONT hFont    = CreateFont( nHeight, 0, 0, 0, dwBold, dwItalic,
                          FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                          VARIABLE_PITCH, m_strFontName );
    if( NULL==hFont )
        return E_FAIL;

    SelectObject( hDC, hbmBitmap );
    SelectObject( hDC, hFont );

    // Set text properties
    SetTextColor( hDC, RGB(255,255,255) );
    SetBkColor(   hDC, 0x00000000 );
    SetTextAlign( hDC, TA_TOP );

    // Loop through all printable character and output them to the bitmap..
    // Meanwhile, keep track of the corresponding tex coords for each character.
    DWORD x = 0;
    DWORD y = 0;
    TCHAR str[2] = _T("x");
    SIZE size;

    // Calculate the spacing between characters based on line height
    GetTextExtentPoint32( hDC, TEXT(" "), 1, &size );
    x = m_dwSpacing = (DWORD) ceil(size.cy * 0.3f);

    for( TCHAR c=32; c < NUMBER_OF_CHARACTERS; c++ )
    {
        str[0] = c;
        GetTextExtentPoint32( hDC, str, 1, &size );

        if( (DWORD)(x + size.cx + m_dwSpacing) > m_dwTexWidth )
        {
            x  = m_dwSpacing;
            y += size.cy+1;
        }

        ExtTextOut( hDC, x+0, y+0, ETO_OPAQUE, NULL, str, 1, NULL );

        m_fTexCoords[c-32][0] = ((FLOAT)(x + 0       - m_dwSpacing))/m_dwTexWidth;
        m_fTexCoords[c-32][1] = ((FLOAT)(y + 0       + 0          ))/m_dwTexHeight;
        m_fTexCoords[c-32][2] = ((FLOAT)(x + size.cx + m_dwSpacing))/m_dwTexWidth;
        m_fTexCoords[c-32][3] = ((FLOAT)(y + size.cy + 0          ))/m_dwTexHeight;

        x += size.cx + (2 * m_dwSpacing);
    }

    // Lock the surface and write the alpha values for the set pixels
    D3DLOCKED_RECT d3dlr;
    LockTexture( &d3dlr );
    BYTE* pDstRow = (BYTE*)d3dlr.pBits;
    
    #if PIXEL_DEPTH == 16
    WORD* pDstPixel;
    #elif PIXEL_DEPTH == 32
    DWORD* pDstPixel;
    #endif
    
    BYTE bAlpha; // pixel intensity

    for( y=0; y < m_dwTexHeight; y++ )
    {
        #if PIXEL_DEPTH == 16
        pDstPixel = (WORD*)pDstRow;
        #elif PIXEL_DEPTH == 32
        pDstPixel = (DWORD*)pDstRow;
        #endif

        for( x=0; x < m_dwTexWidth; x++ )
        {
            #if PIXEL_DEPTH == 16
            bAlpha = (BYTE)((pBitmapBits[m_dwTexWidth*y + x] & 0xff) >> 4);
            #elif PIXEL_DEPTH == 32
            bAlpha = (BYTE)((pBitmapBits[m_dwTexWidth*y + x] & 0xff) >> 0);
            #endif

            if (bAlpha > 0)
            {
                #if PIXEL_DEPTH == 16
                *pDstPixel++ = (WORD) ((bAlpha << 12) | 0x0fff);
                #elif PIXEL_DEPTH == 32
                *pDstPixel++ = (DWORD) ((bAlpha << 24) | 0x00ffffff);
                #endif
            }
            else
            {
                #if PIXEL_DEPTH == 16
                *pDstPixel++ = 0x0000;
                #elif PIXEL_DEPTH == 32
                *pDstPixel++ = 0x00000000;
                #endif
            }
        }

        pDstRow += d3dlr.Pitch;
    }

    // Done updating texture, so clean up used objects
    UnlockTexture();
    DeleteObject( hbmBitmap );
    DeleteDC( hDC );
    DeleteObject( hFont );

    return S_OK;
}


HRESULT
Font::GetTextExtent( TCHAR* strText, SIZE* pSize )
{
    if( NULL==strText || NULL==pSize )
        return E_FAIL;

    FLOAT fRowWidth  = 0.0f;
    FLOAT fRowHeight = (m_fTexCoords[0][3]-m_fTexCoords[0][1])*m_dwTexHeight;
    FLOAT fWidth     = 0.0f;
    FLOAT fHeight    = fRowHeight;

    while( *strText )
    {
        TCHAR c = *strText++;

        if( c == _T('\n') )
        {
            fRowWidth = 0.0f;
            fHeight  += fRowHeight;
        }

        if( (c-32) < 0 || (c-32) >= (NUMBER_OF_CHARACTERS+1) - 32 )
            continue;

        FLOAT tx1 = m_fTexCoords[c-32][0];
        FLOAT tx2 = m_fTexCoords[c-32][2];

        fRowWidth += (tx2-tx1)*m_dwTexWidth - 2*m_dwSpacing;

        if( fRowWidth > fWidth )
            fWidth = fRowWidth;
    }

    pSize->cx = (int)fWidth;
    pSize->cy = (int)fHeight;

    return S_OK;
}


VOID
Font::BuildSpriteQuad( Sprite *sprite, SpriteVertex v[ 4 ] )
{
    FLOAT vertLeft = 2.0f * (float) sprite->DestRect.left / ScreenWidth - 1.0f;
    FLOAT vertRight = 2.0f * (float) sprite->DestRect.right / ScreenWidth - 1.0f;
    FLOAT vertTop = 1.0f - 2.0f * (float) sprite->DestRect.top / ScreenHeight;
    FLOAT vertBottom = 1.0f - 2.0f * (float) sprite->DestRect.bottom / ScreenHeight;
    float tx, ty;
    int i;
    XMVECTOR scaling, origin, translation;
    XMMATRIX T;

    v[ 0 ].x        = vertLeft;
    v[ 0 ].y        = vertBottom;
    v[ 0 ].z        = sprite->Z;
    v[ 0 ].Tex.x    = (float)sprite->SrcRect.left  / m_dwTexWidth;
    v[ 0 ].Tex.y    = (float)sprite->SrcRect.bottom / m_dwTexHeight;
    v[ 0 ].Color    = sprite->Color;

    v[ 1 ].x        = vertLeft;
    v[ 1 ].y        = vertTop;
    v[ 1 ].z        = sprite->Z;
    v[ 1 ].Tex.x    = (float)sprite->SrcRect.left  / m_dwTexWidth;
    v[ 1 ].Tex.y    = (float)sprite->SrcRect.top    / m_dwTexHeight;
    v[ 1 ].Color    = sprite->Color;

    v[ 2 ].x        = vertRight;
    v[ 2 ].y        = vertTop;
    v[ 2 ].z        = sprite->Z;
    v[ 2 ].Tex.x    = (float)sprite->SrcRect.right / m_dwTexWidth;
    v[ 2 ].Tex.y    = (float)sprite->SrcRect.top    / m_dwTexHeight;
    v[ 2 ].Color    = sprite->Color;

    v[ 3 ].x        = vertRight;
    v[ 3 ].y        = vertBottom;
    v[ 3 ].z        = sprite->Z;
    v[ 3 ].Tex.x    = (float)sprite->SrcRect.right / m_dwTexWidth;
    v[ 3 ].Tex.y    = (float)sprite->SrcRect.bottom / m_dwTexHeight;
    v[ 3 ].Color    = sprite->Color;

    tx = 0.5f * ( v[ 0 ].x + v[ 3 ].x );
    ty = 0.5f * ( v[ 0 ].y + v[ 1 ].y );

    scaling        = XMVectorSet( sprite->Scale, sprite->Scale, 1.0f, 0.0f );
    origin            = XMVectorSet( tx, ty, 0.0f, 0.0f );
    translation    = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
    T = XMMatrixAffineTransformation2D( scaling, origin, sprite->Angle, translation );

    for( i = 0; i < 4; ++i )
    {
        XMFLOAT3 xmfloat;
        XMVECTOR p;

        xmfloat.x = v[ i ].x;
        xmfloat.y = v[ i ].y;
        xmfloat.z = v[ i ].z;

        p = XMLoadFloat3( &xmfloat );
        p = XMVector3TransformCoord( p, &T );
        XMStoreFloat3( &xmfloat, p );
        v[ i ].x = xmfloat.x;
        v[ i ].y = xmfloat.y;
        v[ i ].z = xmfloat.z;
    }
}