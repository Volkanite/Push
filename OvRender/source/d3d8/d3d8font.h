#ifndef D3DFONT_H
#define D3DFONT_H
#include <tchar.h>
#include <D3D8.h>
#include "font.h"


//-----------------------------------------------------------------------------
// Name: class CD3DFont
// Desc: Texture-based font class for doing text in a 3D scene.
//-----------------------------------------------------------------------------
class Dx8Font : public Font
{
    LPDIRECT3DDEVICE8      m_pd3dDevice; // A D3DDevice used for rendering
    LPDIRECT3DTEXTURE8     m_pTexture;   // The d3d texture for this font
    LPDIRECT3DVERTEXBUFFER8 m_pVB;        // VertexBuffer for rendering text

    // Stateblocks for setting and restoring render states
    DWORD   m_dwSavedStateBlock;
    DWORD   m_dwDrawTextStateBlock;

public:
    // 2D and 3D text drawing functions
    HRESULT DrawText( FLOAT x, FLOAT y, DWORD dwColor,
                      TCHAR* strText, DWORD dwFlags=0L );

    // Initializing and destroying device-dependent objects
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();

    // Constructor / destructor
    Dx8Font( LPDIRECT3DDEVICE8 pd3dDevice );
    ~Dx8Font();

    DWORD GetMaxTextureWidth();
    HRESULT MapTexture(D3DLOCKED_RECT *pLockedRect);
    HRESULT UnmapTexture();
};




#endif


