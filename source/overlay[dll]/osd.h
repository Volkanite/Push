#include <OvRender.h>


/**
* Draws all on-screen display items.
*/
VOID Osd_Draw(OvOverlay* Overlay);

typedef struct _OSD_VARS
{
    BOOLEAN GraphicsApi;
    BOOLEAN FrameTime;
}OSD_VARS;