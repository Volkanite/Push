#include <OvRender.h>


/**
* Draws all on-screen display items.
*/
VOID Osd_Draw(OvOverlay* Overlay);

typedef struct _OSD_VARS
{
    BOOLEAN GraphicsApi;
    BOOLEAN InputApi;
    BOOLEAN Resolution;
    BOOLEAN Buffers;
    UCHAR FrameTime;
}OSD_VARS;

typedef enum CPU_CALC_INDEX
{
    CPU_CALC_twc,
    CPU_CALC_t,
    CPU_CALC_o,
    CPU_CALC_c

} CPU_CALC_INDEX;