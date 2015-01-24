#ifndef OVERLAY_H
#define OVERLAY_H


typedef class OvOverlay;
typedef VOID (*OV_RENDER)( OvOverlay* Overlay );


typedef struct _OV_HOOK_PARAMS{
    OV_RENDER RenderFunction;
    BOOLEAN ForceVsync;
    BOOLEAN ForceTrippleBuffering;
}OV_HOOK_PARAMS;


class OvOverlay{
public:
    UINT8 Line;
    OV_RENDER UserRenderFunction;
    BOOLEAN ForceVsync;

    OvOverlay();
    VOID Render();

    virtual VOID DrawText( WCHAR* Text ) = 0;
    virtual VOID DrawText( WCHAR* Text, DWORD Color ) = 0;
    virtual VOID DrawText( WCHAR* Text, int X, int Y, DWORD Color ) = 0;
    virtual VOID Begin() = 0;
    virtual VOID End() = 0;
};


VOID
OvCreateOverlay( OV_RENDER RenderFunction );

VOID
OvCreateOverlayEx( OV_HOOK_PARAMS* HookParameters );


#endif
