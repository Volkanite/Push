#include <Windows.h>
#include <time.h>
#include <stdio.h>

#include "overlay.h"
#include "osd.h"


extern UINT32 BackBufferWidth;
extern UINT32 BackBufferHeight;
extern UINT32 BackBufferCount;

extern double FrameTimeAvg;
OSD_VARS Variables;
extern BOOLEAN IsStableFrametime;
extern BOOLEAN IsLimitedFrametime;


CPU_CALC_INDEX CPUStrap = CPU_CALC_twc;

/**
* Draws all on-screen display items.
*/

VOID Osd_Draw( OvOverlay* Overlay )
{
    UINT8 i;
    OSD_ITEM *osdItem;

    osdItem = (OSD_ITEM*) PushSharedMemory->OsdItems;

    for (i = 0; i < PushSharedMemory->NumberOfOsdItems; i++, osdItem++)
    {
        //Process specific.
        switch (osdItem->Flag)
        {
        case OSD_RESOLUTION:
            swprintf(osdItem->Text, 20, L"MON : %i x %i", BackBufferWidth, BackBufferHeight);
            break;
        case OSD_BUFFERS:
            swprintf(osdItem->Text, 20, L"Buffers : %i", BackBufferCount);
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
            || osdItem->Queue)
        {
            Overlay->DrawText(osdItem->Text, osdItem->Color);
        }
    }

    if (Variables.GraphicsApi)
    {
        switch (GraphicsApi)
        {
        case API_OGL:
            Overlay->DrawText(L"API: OGL");
            break;
        case API_DDRAW:
            Overlay->DrawText(L"API: DDRAW");
            break;
        case API_D3D8:
            Overlay->DrawText(L"API: D3D8");
            break;
        case API_D3D9:
            Overlay->DrawText(L"API: D3D9");
            break;
        case API_D3D10:
            Overlay->DrawText(L"API: D3D10");
            break;
        case API_D3D11:
            Overlay->DrawText(L"API: D3D11");
            break;
        }
    }

    if (Variables.FrameTime)
    {
        wchar_t buffer[100];

        switch (Variables.FrameTime)
        {
        case 1:
        case 2:
            swprintf(buffer, 100, L"FrameTime: %.1f", FrameTimeAvg);
            Overlay->DrawText(buffer);
            break;
        }
    }

    if (!IsStableFramerate || PushSharedMemory->KeepFps)
    {
        wchar_t buffer[100];
        DWORD color;

        osdItem->Value = FrameRate;
        //swprintf(buffer, 20, L"%i", FrameRate);
        swprintf(buffer, 100, L"%.0f", FrameRate);

        if (IsStableFrametime)
        {
            color = 0xFFFFFF00;

            if (IsLimitedFrametime)
            {
                color = 0xFFFFA500;
            }
        }
        else
        {
            color = 0xFFFF0000;
        }

        Overlay->DrawText(buffer, color);
    }
}
