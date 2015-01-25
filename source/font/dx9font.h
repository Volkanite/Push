#ifndef D3DFONT_H
#define D3DFONT_H
#include <D3D9.h>
#include "font.h"


//-----------------------------------------------------------------------------
// Name: class CD3DFont
// Desc: Texture-based font class for doing text in a 3D scene.
//-----------------------------------------------------------------------------
class Dx9Font : public Font
{
    LPDIRECT3DDEVICE9       m_pd3dDevice; // A D3DDevice used for rendering
    LPDIRECT3DTEXTURE9      m_pTexture;   // The d3d texture for this font
    LPDIRECT3DVERTEXBUFFER9 m_pVB;        // VertexBuffer for rendering text

    // Stateblocks for setting and restoring render states
    LPDIRECT3DSTATEBLOCK9 m_pStateBlockSaved;
    LPDIRECT3DSTATEBLOCK9 m_pStateBlockDrawText;

public:
    // 2D and 3D text drawing functions
    HRESULT DrawText( FLOAT x, FLOAT y, DWORD dwColor,
                        WCHAR* strText, DWORD dwFlags=0L );
    HRESULT DrawTextScaled( FLOAT x, FLOAT y, FLOAT z,
                            FLOAT fXScale, FLOAT fYScale, DWORD dwColor,
                            const WCHAR* strText, DWORD dwFlags=0L );
    HRESULT Render3DText( const WCHAR* strText, DWORD dwFlags=0L );

    // Initializing and destroying device-dependent objects
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();

    // Constructor / destructor
    Dx9Font( LPDIRECT3DDEVICE9 pd3dDevice );
    ~Dx9Font();

    // Standard functions
    DWORD GetMaxTextureWidth();
    HRESULT CreateTexture();
    VOID LockTexture( D3DLOCKED_RECT *pLockedRect );
    VOID UnlockTexture();
};



#endif
