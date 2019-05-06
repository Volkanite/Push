#ifndef OVERLAY_H
#define OVERLAY_H

#include <Windows.h>

class OvOverlay;
typedef VOID (*OV_RENDER)( OvOverlay* Overlay );

typedef enum OV_VSYNC_OVERRIDE_MODE
{
    VSYNC_UNCHANGED = 0,
    VSYNC_FORCE_ON,
    VSYNC_FORCE_OFF,

} OV_VSYNC_OVERRIDE_MODE;

typedef enum OV_WINDOW_MODE
{
    WINDOW_UNCHANGED = 0,
    WINDOW_FULLSCREEN,
    WINDOW_WINDOWED,

} OV_WINDOW_MODE;

typedef enum OV_GRAPHICS_API
{
    API_OGL,
    API_DDRAW,
    API_D3D8,
    API_D3D9,
    API_D3D10,
    API_D3D11
    
} OV_GRAPHICS_API;

#define OV_D3D8            0x00000001
#define OV_D3D9        0x00000002
#define OV_DXGI        0x00000004
#define OV_DDRAW        0x00000008
#define OV_OGL        0x00000010

typedef struct OV_HOOK_PARAMS{
    OV_RENDER               RenderFunction;
    OV_VSYNC_OVERRIDE_MODE  VsyncOverrideMode;
    BOOLEAN                 ForceTrippleBuffering;
    WCHAR*                  FontName;
    BOOLEAN                 FontBold;
    DWORD                   InterfaceFlags;
}OV_HOOK_PARAMS;

typedef struct _FONT_PROPERTIES{
    WCHAR* Name;
    BOOLEAN Bold;
    UINT32 Size;
}FONT_PROPERTIES;

class OvOverlay{
public:
    UINT8                   Line;
    OV_RENDER               UserRenderFunction;
    OV_VSYNC_OVERRIDE_MODE  VsyncOverrideMode;
    FONT_PROPERTIES         FontProperties;
    UINT32 BackBufferWidth;
    UINT32 BackBufferHeight;

    OvOverlay();
    VOID Render();

    virtual VOID DrawText( WCHAR* Text ) = 0;
    virtual VOID DrawText( WCHAR* Text, DWORD Color ) = 0;
    virtual VOID DrawText( WCHAR* Text, int X, int Y, DWORD Color ) = 0;
    virtual VOID Begin() = 0;
    virtual VOID End() = 0;
    virtual VOID* GetDevice() = 0;
};


VOID OvCreateOverlay( OV_HOOK_PARAMS* HookParameters );
VOID DestroyOverlay();

VOID OvLog(const wchar_t* Format, ...);

//extern UINT32 BackBufferWidth;
//extern UINT32 BackBufferHeight;
extern OV_GRAPHICS_API GraphicsApi;


#endif
