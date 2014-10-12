#include <windows.h>
#include "overlay.h"
#include "dx8/dx8overlay.h"
#include "dx9/dx9overlay.h"
#include "dxgi/dxgioverlay.h"


Dx8Overlay*     OvDx8Overlay;
Dx9Overlay*     OvDx9Overlay;
DxgiOverlay*    OvDxgiOverlay;

OV_RENDER OvUserRenderFunction;


OvOverlay::OvOverlay()
{
    Line = 0;
}


VOID
OvOverlay::Render()
{
    Line = 0;

    Begin();
    UserRenderFunction( this );
    End();
}


ULONG __stdcall CreateOverlay( LPVOID Param )
{
	if (GetModuleHandleA("d3d8.dll"))
    {
        if (OvDx8Overlay == NULL)
            OvDx8Overlay = new Dx8Overlay( OvUserRenderFunction );
    }

    if (GetModuleHandleA("d3d9.dll"))
    {
        if (OvDx9Overlay == NULL)
            OvDx9Overlay = new Dx9Overlay( OvUserRenderFunction );
    }

    if (GetModuleHandleA("dxgi.dll"))
    {
        if (OvDxgiOverlay == NULL)
            OvDxgiOverlay = new DxgiOverlay( OvUserRenderFunction );
    }

    return NULL;
}


VOID
OvCreateOverlay( OV_RENDER RenderFunction )
{
	OvUserRenderFunction = RenderFunction;

	CreateThread(0, 0, &CreateOverlay, 0, 0, 0);
}
