#include <windows.h>
#include <tchar.h>

#include "font.h"


extern "C" VOID* __stdcall RtlReAllocateHeap(
    VOID*   HeapHandle,
    DWORD   Flags,
    VOID*   MemoryPointer,
    UINT32  Size
    );


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


VOID Font::SetFontAttributes( FONT_PROPERTIES* FontProperties )
{
    _tcsncpy(
        m_strFontName,
        FontProperties->Name,
        sizeof(m_strFontName) / sizeof(TCHAR)
        );

    m_strFontName[sizeof(m_strFontName) / sizeof(TCHAR) - 1] = _T('\0');

    if (FontProperties->Bold)
        m_dwFontFlags = D3DFONT_BOLD;
    else
        m_dwFontFlags = 0;

    m_dwFontHeight = FontProperties->Size;
}


VOID Font::WriteAlphaValuesFromBitmapToTexture( DWORD* Bitmap, void* Texture, int Pitch )
{
    DWORD x = 0;
    DWORD y = 0;
    BYTE* pDstRow = (BYTE*)Texture;
    
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
            bAlpha = (BYTE)((Bitmap[m_dwTexWidth*y + x] & 0xff) >> 0);
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

        pDstRow += Pitch;
    }
}


HBITMAP Font::CreateFontBitmap( DWORD MaxTextureWidth, DWORD** Bitmap )
{
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

    if (m_dwTexWidth > MaxTextureWidth)
    {
        m_fTextScale = (FLOAT)MaxTextureWidth / (FLOAT)m_dwTexWidth;
        m_dwTexWidth = m_dwTexHeight = MaxTextureWidth;
    }

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
        return NULL;

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

    // Clean up used objects
    //DeleteObject( hbmBitmap );
    DeleteDC( hDC );
    DeleteObject( hFont );

    *Bitmap = pBitmapBits;

    return hbmBitmap;
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


UINT  NumberOfSprites;
UINT  SpriteListSize;
Sprite* Sprites;
VOID*   HeapHandle;


VOID AddSprite( Sprite *sprite )
{
    if ( (NumberOfSprites + 1) * sizeof(Sprite) > SpriteListSize )
        // resize sprite list
    {
        Sprites = (Sprite*) HeapReAlloc(
            HeapHandle,
            HEAP_GENERATE_EXCEPTIONS,
            Sprites,
            SpriteListSize + sizeof(Sprite)
            );

        //adjust sprite list size counter
        SpriteListSize += sizeof(Sprite);
    }

    if (!Sprites)
        return;

    //add sprite to sprite list
    Sprites[NumberOfSprites].Angle    = sprite->Angle;
    Sprites[NumberOfSprites].Color    = sprite->Color;
    Sprites[NumberOfSprites].DestRect = sprite->DestRect;
    Sprites[NumberOfSprites].Scale    = sprite->Scale;
    Sprites[NumberOfSprites].SrcRect  = sprite->SrcRect;
    Sprites[NumberOfSprites].Z        = sprite->Z;

    NumberOfSprites++;
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


VOID Draw( RECT* destinationRect, RECT* sourceRect, XMCOLOR color )
{
    Sprite sprite;
    sprite.SrcRect = *sourceRect;
    sprite.DestRect = *destinationRect;
    sprite.Color = color;
    sprite.Z = 0.0f;
    sprite.Angle = 0.0f;
    sprite.Scale = 1.0f;

    AddSprite(&sprite);
}


VOID Font::AddString( WCHAR *text, DWORD Color )
{
    UINT i, length = (UINT)wcslen(text);
    int xbackup = posX;
    XMCOLOR color;

    color.c = Color;

    for (i = 0; i < length; ++i)
    {
        WCHAR character = text[i];

        if (character == ' ')
        {
            posX += m_dwSpacing;
        }
        else if (character == '\n')
        {
            posX = xbackup;
            posY += m_dwFontHeight;
        }
        else if (character > NUMBER_OF_CHARACTERS)
        {
            break;
        }
        else
        {
            RECT charRect;

            charRect.left = (LONG)(((m_fTexCoords[character - 32][0]) * m_dwTexWidth) + m_dwSpacing);
            charRect.top = (LONG)((m_fTexCoords[character - 32][1]) * m_dwTexWidth);
            charRect.right = (LONG)(((m_fTexCoords[character - 32][2]) * m_dwTexWidth) - m_dwSpacing);
            charRect.bottom = (LONG)((m_fTexCoords[character - 32][3]) * m_dwTexWidth);

            int width = charRect.right - charRect.left;
            int height = charRect.bottom - charRect.top;
            RECT rect = { posX, posY, posX + width, posY + height };

            Draw(&rect, &charRect, color);

            posX += width + 1;
        }
    }
}


VOID Font::DrawText(FLOAT sx, FLOAT sy, DWORD Color, WCHAR* Text)
{
    posX = (int)sx;
    posY = (int)sy;

    AddString(Text, Color);
}