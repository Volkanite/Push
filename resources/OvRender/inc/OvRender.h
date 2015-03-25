#ifndef OVERLAY_H
#define OVERLAY_H


typedef class OvOverlay;
typedef VOID (*OV_RENDER)( OvOverlay* Overlay );

typedef enum _OV_VSYNC_OVERRIDE_MODE
{
    VSYNC_UNCHANGED = 0,
    VSYNC_FORCE_ON,
    VSYNC_FORCE_OFF,

} OV_VSYNC_OVERRIDE_MODE;

typedef struct _OV_HOOK_PARAMS{
    OV_RENDER               RenderFunction;
    OV_VSYNC_OVERRIDE_MODE  VsyncOverrideMode;
    BOOLEAN                 ForceTrippleBuffering;
}OV_HOOK_PARAMS;


class OvOverlay{
public:
    UINT8                   Line;
    OV_RENDER               UserRenderFunction;
    OV_VSYNC_OVERRIDE_MODE  VsyncOverrideMode;

    OvOverlay();
    VOID Render();

    virtual VOID DrawText( WCHAR* Text ) = 0;
    virtual VOID DrawText( WCHAR* Text, DWORD Color ) = 0;
    virtual VOID DrawText( WCHAR* Text, int X, int Y, DWORD Color ) = 0;
    virtual VOID Begin() = 0;
    virtual VOID End() = 0;
    virtual VOID* GetDevice() = 0;
};


VOID
OvCreateOverlay( OV_RENDER RenderFunction );

VOID
OvCreateOverlayEx( OV_HOOK_PARAMS* HookParameters );


#endif
