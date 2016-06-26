#include <Windows.h>
#include <time.h>
#include <stdio.h>

#include "overlay.h"
#include <OvRender.h>

extern UINT32 BackBufferWidth;
extern UINT32 BackBufferHeight;


/**
* Draws all on-screen display items.
*/

VOID Osd_Draw( OvOverlay* Overlay )
{
    UINT8 i;
    OSD_ITEM *osdItem;
    osdItem = PushSharedMemory->OsdItems;

    for (i = 0; i < PushSharedMemory->NumberOfOsdItems; i++, osdItem++)
    {
        //Process specific.
        switch (osdItem->Flag)
        {
        case OSD_FPS:
            {
                if (!IsStableFramerate)
                {
                    osdItem->Value = FrameRate;
                    swprintf(osdItem->Text, 20, L"%i", osdItem->Value);
                }
                else
                {
                    continue;
                }
            }
            break;
        case OSD_RESOLUTION:
            swprintf(osdItem->Text, 20, L"RES : %i x %i", BackBufferWidth, BackBufferHeight);
            break;
        case OSD_DISK_RESPONSE:
            osdItem->Value = DiskResponseTime;
            swprintf(osdItem->Text, 20, L"DSK : %i ms", osdItem->Value);
            break;
        default:
            break;
        }

        if (!osdItem->Flag //draw if no flag, could be somebody just wants to display stuff on-screen
            || PushSharedMemory->OSDFlags & osdItem->Flag //if it has a flag, is it set?
            || (osdItem->Threshold && osdItem->Value > osdItem->Threshold)) //is the item's value > it's threshold?
        {
            Overlay->DrawText(osdItem->Text, osdItem->Color);
        }
    }
}
