#ifndef OVERLAY_H
#define OVERLAY_H


typedef class OvOverlay;
typedef VOID (*OV_RENDER)( OvOverlay* Overlay );


// Primitives supported by draw-primitive API
typedef enum _OVPRIMITIVETYPE {
    OVPT_POINTLIST             = 1,
    OVPT_LINELIST              = 2,
    OVPT_LINESTRIP             = 3,
    OVPT_TRIANGLELIST          = 4,
    OVPT_TRIANGLESTRIP         = 5,
    OVPT_TRIANGLEFAN           = 6,
} OVPRIMITIVETYPE;

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
    virtual VOID DrawPrimitiveUP( OVPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *VertexData, UINT VertexSize ) = 0;
    virtual VOID Begin() = 0;
    virtual VOID End() = 0;
};


VOID
OvCreateOverlay( OV_RENDER RenderFunction );

VOID
OvCreateOverlayEx( OV_HOOK_PARAMS* HookParameters );


#endif
