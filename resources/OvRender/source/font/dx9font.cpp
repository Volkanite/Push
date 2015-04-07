#include <stdio.h>
#include <tchar.h>
#include <D3DX9.h>

#include "dx9font.h"


//-----------------------------------------------------------------------------
// Custom vertex types for rendering text
//-----------------------------------------------------------------------------
#define MAX_NUM_VERTICES 50*6

struct FONT2DVERTEX { D3DXVECTOR4 p;   DWORD color;     FLOAT tu, tv; };
struct FONT3DVERTEX { D3DXVECTOR3 p;   D3DXVECTOR3 n;   FLOAT tu, tv; };

#define D3DFVF_FONT2DVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define D3DFVF_FONT3DVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

inline FONT2DVERTEX InitFont2DVertex( const D3DXVECTOR4& p, D3DCOLOR color,
                                      FLOAT tu, FLOAT tv )
{
    FONT2DVERTEX v;   v.p = p;   v.color = color;   v.tu = tu;   v.tv = tv;
    return v;
}

inline FONT3DVERTEX InitFont3DVertex( const D3DXVECTOR3& p, const D3DXVECTOR3& n,
                                      FLOAT tu, FLOAT tv )
{
    FONT3DVERTEX v;   v.p = p;   v.n = n;   v.tu = tu;   v.tv = tv;
    return v;
}


//-----------------------------------------------------------------------------
// Name: CD3DFont()
// Desc: Font class constructor
//-----------------------------------------------------------------------------
Dx9Font::Dx9Font(
    LPDIRECT3DDEVICE9 pd3dDevice
    )
{
    D3D9Font_Device           = NULL;
    m_pTexture             = NULL;
    m_pVB                  = NULL;

    //m_pStateBlockSaved     = NULL;
    //m_pStateBlockDrawText  = NULL;

    // Keep a local copy of the device
    D3D9Font_Device = pd3dDevice;
}



//-----------------------------------------------------------------------------
// Name: ~CD3DFont()
// Desc: Font class destructor
//-----------------------------------------------------------------------------
Dx9Font::~Dx9Font()
{
    InvalidateDeviceObjects();
    DeleteDeviceObjects();
}


DWORD
Dx9Font::GetMaxTextureWidth()
{
    D3DCAPS9 d3dCaps;

    D3D9Font_Device->GetDeviceCaps( &d3dCaps );

    return d3dCaps.MaxTextureWidth;
}


HRESULT
Dx9Font::CreateTexture()
{
    HRESULT hr;
    D3DFORMAT format;

    if (PIXEL_DEPTH == 16)
        format = D3DFMT_A4R4G4B4;
    else if (PIXEL_DEPTH == 32)
        format = D3DFMT_A8R8G8B8;

    hr = D3D9Font_Device->CreateTexture(
        m_dwTexWidth, 
        m_dwTexHeight, 
        1, 
        D3DUSAGE_DYNAMIC, 
        format, 
        D3DPOOL_DEFAULT, 
        &m_pTexture, 
        NULL
        );

    return hr;
}


VOID
Dx9Font::LockTexture(
    D3DLOCKED_RECT* pLockedRect
    )
{
    m_pTexture->LockRect( 0, pLockedRect, 0, 0 );
}


VOID
Dx9Font::UnlockTexture()
{
    m_pTexture->UnlockRect(0);
}


//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT Dx9Font::RestoreDeviceObjects()
{
    HRESULT hr;

    // Create vertex buffer for the letters
    int vertexSize = max( sizeof(FONT2DVERTEX), sizeof(FONT3DVERTEX ) );
    if( FAILED( hr = D3D9Font_Device->CreateVertexBuffer( MAX_NUM_VERTICES * vertexSize,
                                                       D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0,
                                                       D3DPOOL_DEFAULT, &m_pVB, NULL ) ) )
    {
        return hr;
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Destroys all device-dependent objects
//-----------------------------------------------------------------------------
HRESULT Dx9Font::InvalidateDeviceObjects()
{
    if (m_pVB)
        m_pVB->Release();

    m_pVB = NULL;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Destroys all device-dependent objects
//-----------------------------------------------------------------------------
HRESULT Dx9Font::DeleteDeviceObjects()
{
    if (m_pTexture)
        m_pTexture->Release();

    m_pTexture = NULL;
    D3D9Font_Device = NULL;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: DrawText()
// Desc: Draws 2D text. Note that sx and sy are in pixels
//-----------------------------------------------------------------------------
HRESULT Dx9Font::DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, TCHAR* strText, DWORD dwFlags )
{
    DWORD D3DRENDERSTATETYPE_D3DRS_ALPHABLENDENABLE;
    DWORD D3DRENDERSTATETYPE_D3DRS_SRCBLEND;
    DWORD D3DRENDERSTATETYPE_D3DRS_DESTBLEND;
    DWORD D3DRENDERSTATETYPE_D3DRS_ALPHATESTENABLE;
    DWORD D3DRENDERSTATETYPE_D3DRS_ALPHAREF;
    DWORD D3DRENDERSTATETYPE_D3DRS_ALPHAFUNC;
    DWORD D3DRENDERSTATETYPE_D3DRS_FILLMODE;
    DWORD D3DRENDERSTATETYPE_D3DRS_CULLMODE;
    DWORD D3DRENDERSTATETYPE_D3DRS_STENCILENABLE;
    DWORD D3DRENDERSTATETYPE_D3DRS_CLIPPING;
    DWORD D3DRENDERSTATETYPE_D3DRS_CLIPPLANEENABLE;
    DWORD D3DRENDERSTATETYPE_D3DRS_VERTEXBLEND;
    DWORD D3DRENDERSTATETYPE_D3DRS_INDEXEDVERTEXBLENDENABLE;
    DWORD D3DRENDERSTATETYPE_D3DRS_FOGENABLE;
    DWORD D3DRENDERSTATETYPE_D3DRS_COLORWRITEENABLE;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_COLOROP_0;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_COLORARG1;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_COLORARG2;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAOP_0;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAARG1;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAARG2;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_TEXCOORDINDEX;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_TEXTURETRANSFORMFLAGS;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_COLOROP_1;
    DWORD D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAOP_1;
    DWORD D3DSAMPLERSTATETYPE_D3DSAMP_MINFILTER;
    DWORD D3DSAMPLERSTATETYPE_D3DSAMP_MAGFILTER;
    DWORD D3DSAMPLERSTATETYPE_D3DSAMP_MIPFILTER;
    DWORD FVF;
    IDirect3DBaseTexture9* texture = NULL;
    IDirect3DVertexBuffer9* vertexBuffer = NULL;
    UINT stride;
    UINT offset;
    
    if( D3D9Font_Device == NULL )
        return E_FAIL;
    
    // Save device state.
    
    D3D9Font_Device->GetRenderState(D3DRS_ALPHABLENDENABLE, &D3DRENDERSTATETYPE_D3DRS_ALPHABLENDENABLE);
    D3D9Font_Device->GetRenderState(D3DRS_SRCBLEND, &D3DRENDERSTATETYPE_D3DRS_SRCBLEND);
    D3D9Font_Device->GetRenderState(D3DRS_DESTBLEND, &D3DRENDERSTATETYPE_D3DRS_DESTBLEND);
    D3D9Font_Device->GetRenderState(D3DRS_ALPHATESTENABLE, &D3DRENDERSTATETYPE_D3DRS_ALPHATESTENABLE);
    D3D9Font_Device->GetRenderState(D3DRS_ALPHAREF, &D3DRENDERSTATETYPE_D3DRS_ALPHAREF);
    D3D9Font_Device->GetRenderState(D3DRS_ALPHAFUNC, &D3DRENDERSTATETYPE_D3DRS_ALPHAFUNC);
    D3D9Font_Device->GetRenderState(D3DRS_FILLMODE, &D3DRENDERSTATETYPE_D3DRS_FILLMODE);
    D3D9Font_Device->GetRenderState(D3DRS_CULLMODE, &D3DRENDERSTATETYPE_D3DRS_CULLMODE);
    D3D9Font_Device->GetRenderState(D3DRS_STENCILENABLE, &D3DRENDERSTATETYPE_D3DRS_STENCILENABLE);
    D3D9Font_Device->GetRenderState(D3DRS_CLIPPING, &D3DRENDERSTATETYPE_D3DRS_CLIPPING);
    D3D9Font_Device->GetRenderState(D3DRS_CLIPPLANEENABLE, &D3DRENDERSTATETYPE_D3DRS_CLIPPLANEENABLE);
    D3D9Font_Device->GetRenderState(D3DRS_VERTEXBLEND, &D3DRENDERSTATETYPE_D3DRS_VERTEXBLEND);
    D3D9Font_Device->GetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, &D3DRENDERSTATETYPE_D3DRS_INDEXEDVERTEXBLENDENABLE);
    D3D9Font_Device->GetRenderState(D3DRS_FOGENABLE, &D3DRENDERSTATETYPE_D3DRS_FOGENABLE);
    D3D9Font_Device->GetRenderState(D3DRS_COLORWRITEENABLE, &D3DRENDERSTATETYPE_D3DRS_COLORWRITEENABLE);
    D3D9Font_Device->GetTextureStageState(0, D3DTSS_COLOROP, &D3DTEXTURESTAGESTATETYPE_D3DTSS_COLOROP_0);
    D3D9Font_Device->GetTextureStageState(0, D3DTSS_COLORARG1, &D3DTEXTURESTAGESTATETYPE_D3DTSS_COLORARG1);
    D3D9Font_Device->GetTextureStageState(0, D3DTSS_COLORARG2, &D3DTEXTURESTAGESTATETYPE_D3DTSS_COLORARG2);
    D3D9Font_Device->GetTextureStageState(0, D3DTSS_ALPHAOP, &D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAOP_0);
    D3D9Font_Device->GetTextureStageState(0, D3DTSS_ALPHAARG1, &D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAARG1);
    D3D9Font_Device->GetTextureStageState(0, D3DTSS_ALPHAARG2, &D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAARG2);
    D3D9Font_Device->GetTextureStageState(0, D3DTSS_TEXCOORDINDEX, &D3DTEXTURESTAGESTATETYPE_D3DTSS_TEXCOORDINDEX);
    D3D9Font_Device->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &D3DTEXTURESTAGESTATETYPE_D3DTSS_TEXTURETRANSFORMFLAGS);
    D3D9Font_Device->GetTextureStageState(1, D3DTSS_COLOROP, &D3DTEXTURESTAGESTATETYPE_D3DTSS_COLOROP_1);
    D3D9Font_Device->GetTextureStageState(1, D3DTSS_ALPHAOP, &D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAOP_1);
    D3D9Font_Device->GetSamplerState(0, D3DSAMP_MINFILTER, &D3DSAMPLERSTATETYPE_D3DSAMP_MINFILTER);
    D3D9Font_Device->GetSamplerState(0, D3DSAMP_MAGFILTER, &D3DSAMPLERSTATETYPE_D3DSAMP_MAGFILTER);
    D3D9Font_Device->GetSamplerState(0, D3DSAMP_MIPFILTER, &D3DSAMPLERSTATETYPE_D3DSAMP_MIPFILTER);
    D3D9Font_Device->GetFVF(&FVF);
    D3D9Font_Device->GetTexture(0, &texture);
    D3D9Font_Device->GetStreamSource(0, &vertexBuffer, &offset, &stride);

    // Set new state.
    
    if (D3DFONT_ZENABLE & m_dwFontFlags)
        D3D9Font_Device->SetRenderState(D3DRS_ZENABLE, TRUE);
    else
        D3D9Font_Device->SetRenderState(D3DRS_ZENABLE, FALSE);

    D3D9Font_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    D3D9Font_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    D3D9Font_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    D3D9Font_Device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    D3D9Font_Device->SetRenderState(D3DRS_ALPHAREF, 0x08);
    D3D9Font_Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    D3D9Font_Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    D3D9Font_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    D3D9Font_Device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    D3D9Font_Device->SetRenderState(D3DRS_CLIPPING, TRUE);
    D3D9Font_Device->SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE);
    D3D9Font_Device->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    D3D9Font_Device->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
    D3D9Font_Device->SetRenderState(D3DRS_FOGENABLE, FALSE);
    D3D9Font_Device->SetRenderState(D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
        D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    D3D9Font_Device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    D3D9Font_Device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    D3D9Font_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    D3D9Font_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    D3D9Font_Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    D3D9Font_Device->SetFVF(D3DFVF_FONT2DVERTEX);
    D3D9Font_Device->SetPixelShader(NULL);
    D3D9Font_Device->SetStreamSource( 0, m_pVB, 0, sizeof(FONT2DVERTEX) );
    D3D9Font_Device->SetTexture(0, m_pTexture);
    
    // Set filter states
    if( dwFlags & D3DFONT_FILTERED )
    {
        D3D9Font_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        D3D9Font_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    }

    if( dwFlags & D3DFONT_RIGHT ) {
        SIZE sz;
        GetTextExtent( strText, &sz );
        sx -= (FLOAT)sz.cx;
    } else if( dwFlags & D3DFONT_CENTERED ) {
        SIZE sz;
        GetTextExtent( strText, &sz );
        sx -= (FLOAT)sz.cx/2.0f;
    }

    // Adjust for character spacing
    sx -= m_dwSpacing;
    FLOAT fStartX = sx;

    // Fill vertex buffer
    FONT2DVERTEX* pVertices = NULL;
    DWORD         dwNumTriangles = 0;
    m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );

    while( *strText )
    {
        TCHAR c = *strText++;

        if( c == _T('\n') )
        {
            sx = fStartX;
            sy += (m_fTexCoords[0][3]-m_fTexCoords[0][1])*m_dwTexHeight;
        }

        if( (c-32) < 0 || (c-32) >= (NUMBER_OF_CHARACTERS+1)-32 )
            continue;

        FLOAT tx1 = m_fTexCoords[c-32][0];
        FLOAT ty1 = m_fTexCoords[c-32][1];
        FLOAT tx2 = m_fTexCoords[c-32][2];
        FLOAT ty2 = m_fTexCoords[c-32][3];

        FLOAT w = (tx2-tx1) *  m_dwTexWidth / m_fTextScale;
        FLOAT h = (ty2-ty1) * m_dwTexHeight / m_fTextScale;

        if( c != _T(' ') )
        {
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+h-0.5f,0.9f,1.0f), dwColor, tx1, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+0-0.5f,0.9f,1.0f), dwColor, tx1, ty1 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+h-0.5f,0.9f,1.0f), dwColor, tx2, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+0-0.5f,0.9f,1.0f), dwColor, tx2, ty1 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+h-0.5f,0.9f,1.0f), dwColor, tx2, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+0-0.5f,0.9f,1.0f), dwColor, tx1, ty1 );
            dwNumTriangles += 2;

            if( dwNumTriangles*3 > (MAX_NUM_VERTICES-6) )
            {
                // Unlock, render, and relock the vertex buffer
                m_pVB->Unlock();
                D3D9Font_Device->DrawPrimitive( D3DPT_TRIANGLELIST, 0, dwNumTriangles );
                pVertices = NULL;
                m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );
                dwNumTriangles = 0L;
            }
        }

        sx += w - (2 * m_dwSpacing);
    }

    // Unlock and render the vertex buffer
    m_pVB->Unlock();
    if( dwNumTriangles > 0 )
        D3D9Font_Device->DrawPrimitive( D3DPT_TRIANGLELIST, 0, dwNumTriangles );

    // Restore device state.

    D3D9Font_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, D3DRENDERSTATETYPE_D3DRS_ALPHABLENDENABLE);
    D3D9Font_Device->SetRenderState(D3DRS_SRCBLEND, D3DRENDERSTATETYPE_D3DRS_SRCBLEND);
    D3D9Font_Device->SetRenderState(D3DRS_DESTBLEND, D3DRENDERSTATETYPE_D3DRS_DESTBLEND);
    D3D9Font_Device->SetRenderState(D3DRS_ALPHATESTENABLE, D3DRENDERSTATETYPE_D3DRS_ALPHATESTENABLE);
    D3D9Font_Device->SetRenderState(D3DRS_ALPHAREF, D3DRENDERSTATETYPE_D3DRS_ALPHAREF);
    D3D9Font_Device->SetRenderState(D3DRS_ALPHAFUNC, D3DRENDERSTATETYPE_D3DRS_ALPHAFUNC);
    D3D9Font_Device->SetRenderState(D3DRS_FILLMODE, D3DRENDERSTATETYPE_D3DRS_FILLMODE);
    D3D9Font_Device->SetRenderState(D3DRS_CULLMODE, D3DRENDERSTATETYPE_D3DRS_CULLMODE);
    D3D9Font_Device->SetRenderState(D3DRS_STENCILENABLE, D3DRENDERSTATETYPE_D3DRS_STENCILENABLE);
    D3D9Font_Device->SetRenderState(D3DRS_CLIPPING, D3DRENDERSTATETYPE_D3DRS_CLIPPING);
    D3D9Font_Device->SetRenderState(D3DRS_CLIPPLANEENABLE, D3DRENDERSTATETYPE_D3DRS_CLIPPLANEENABLE);
    D3D9Font_Device->SetRenderState(D3DRS_VERTEXBLEND, D3DRENDERSTATETYPE_D3DRS_VERTEXBLEND);
    D3D9Font_Device->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, D3DRENDERSTATETYPE_D3DRS_INDEXEDVERTEXBLENDENABLE);
    D3D9Font_Device->SetRenderState(D3DRS_FOGENABLE, D3DRENDERSTATETYPE_D3DRS_FOGENABLE);
    D3D9Font_Device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DRENDERSTATETYPE_D3DRS_COLORWRITEENABLE);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTEXTURESTAGESTATETYPE_D3DTSS_COLOROP_0);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTEXTURESTAGESTATETYPE_D3DTSS_COLORARG1);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTEXTURESTAGESTATETYPE_D3DTSS_COLORARG2);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAOP_0);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAARG1);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAARG2);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTEXTURESTAGESTATETYPE_D3DTSS_TEXCOORDINDEX);
    D3D9Font_Device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTEXTURESTAGESTATETYPE_D3DTSS_TEXTURETRANSFORMFLAGS);
    D3D9Font_Device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTEXTURESTAGESTATETYPE_D3DTSS_COLOROP_1);
    D3D9Font_Device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTEXTURESTAGESTATETYPE_D3DTSS_ALPHAOP_1);
    D3D9Font_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DSAMPLERSTATETYPE_D3DSAMP_MINFILTER);
    D3D9Font_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DSAMPLERSTATETYPE_D3DSAMP_MAGFILTER);
    D3D9Font_Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DSAMPLERSTATETYPE_D3DSAMP_MIPFILTER);
    D3D9Font_Device->SetFVF(FVF);
    D3D9Font_Device->SetTexture(0, texture);
    D3D9Font_Device->SetStreamSource(0, vertexBuffer, offset, stride);
        
    return S_OK;
}
