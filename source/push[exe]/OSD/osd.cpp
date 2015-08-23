#include <sl.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <push.h>
#include <hardware.h>

#include "rtss.h"


VOID FormatDiskReadWriteRate(
    UINT32 Value,
    WCHAR* Buffer
    );

VOID FormatTime(
    UINT32 Value,
    WCHAR* Buffer
    );

typedef VOID(*OSD_DYNAMIC_FORMAT)(
    UINT32 Value,
    WCHAR* Buffer
    );


OSD_ITEM OsdItems[] = {
    { OSD_GPU_LOAD, 90, L"GPU : %i %%"},
    { OSD_GPU_TEMP, 75, L"GPU : %i °C"},
    { OSD_GPU_E_CLK, 0, L"GPU : %i MHz"},
    { OSD_GPU_M_CLK, 0, L"GPU : %i MHz"},
    { OSD_GPU_VRAM, 90, L"VRAM : %i MB"},
    { OSD_GPU_FAN, 0, L"GPU : %i RPM"},
    { OSD_RAM, 90, L"RAM : %i MB"},
    { OSD_CPU_LOAD, 95, L"CPU : %i %%"},
    { OSD_CPU_TEMP, 75, L"CPU : %i °C"},
    { OSD_MCU, 0, L"CPUm : %i %%"},
    { OSD_MTU, 0, L"MTU : %i %%"},
    { OSD_DISK_RWRATE, 0, NULL, NULL, NULL, FormatDiskReadWriteRate },
    { OSD_DISK_RESPONSE, 4000, L"DSK : %i ms"},
    { OSD_TIME, 0, NULL, NULL, NULL, FormatTime },
    { OSD_BUFFERS, 0, L"Buffers : %i"},
    { OSD_FPS, 0, L"%i"}
};


VOID GetValues( OSD_ITEM* Item )
{
    switch (Item->Flag)
    {
    case OSD_GPU_LOAD: 
        Item->Value = hardware.DisplayDevice.Load;
        Item->ValueOverride = NULL;
        break;
    case OSD_GPU_TEMP: 
        Item->Value = hardware.DisplayDevice.Temperature;
        Item->ValueOverride = NULL;
        break;
    case OSD_GPU_E_CLK:
        Item->Value = NULL;
        Item->ValueOverride = hardware.DisplayDevice.EngineClock;
        break;
    case OSD_GPU_M_CLK: 
        Item->Value = NULL;
        Item->ValueOverride = hardware.DisplayDevice.MemoryClock;
        break;
    case OSD_GPU_VRAM: 
        Item->Value = hardware.DisplayDevice.FrameBuffer.Load;
        Item->ValueOverride = hardware.DisplayDevice.FrameBuffer.Used;
        break;
    case OSD_GPU_FAN:
        Item->Value = hardware.DisplayDevice.FanSpeed;
        Item->ValueOverride = NULL;
        break;
    case OSD_RAM: 
        Item->Value = hardware.Memory.Load;
        Item->ValueOverride = hardware.Memory.Used;
        break;
    case OSD_CPU_LOAD: 
        Item->Value = hardware.Processor.Load;
        Item->ValueOverride = NULL;
        break;
    case OSD_CPU_TEMP: 
        Item->Value = hardware.Processor.Temperature;
        Item->ValueOverride = NULL;
        break;
    case OSD_MCU: 
        Item->Value = hardware.Processor.MaxCoreUsage;
        Item->ValueOverride = NULL;
        break;
    case OSD_MTU: 
        Item->Value = hardware.Processor.MaxThreadUsage;
        Item->ValueOverride = NULL;
        break;
    case OSD_DISK_RWRATE: 
        Item->Value = NULL;
        Item->ValueOverride = hardware.Disk.ReadWriteRate;
        break;
    case OSD_DISK_RESPONSE: 
        //Item->Value = hardware.Disk.ResponseTime;
        Item->ValueOverride = NULL;
        break;
    case OSD_TIME: 
        Item->Value = NULL;
        Item->ValueOverride = NULL;
        break;
    case OSD_BUFFERS: 
        Item->Value = PushSharedMemory->FrameBufferCount;
        Item->ValueOverride = NULL;
        break;
    case OSD_FPS: 
        Item->Value = PushSharedMemory->IsFrameRateStable;
        Item->ValueOverride = PushSharedMemory->FrameRate;
        break;
    }
}


VOID FormatTime(
    UINT32 Value,
    WCHAR* Buffer
    )
{
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);

    timeinfo = localtime(
        &rawtime
        );

    wcsftime(
        Buffer,
        20,
        L"%H:%M:%S",
        timeinfo
        );
}


VOID FormatDiskReadWriteRate(
    UINT32 Value,
    WCHAR* Buffer
    )
{
    UINT32 rate;
    WCHAR *format;

    rate = Value;

    format = L"DSK : %i B/s";

    if (rate > 1024)
    {
        rate /= 1024;//kB/s
        format = L"DSK : %i kB/s";
    }
    if (rate > 1024)
    {
        rate /= 1024;//MB/s
        format = L"DSK : %i MB/s";
    }

    swprintf(
        Buffer,
        20,
        format,
        rate
        );
}


/**
* Refreshes all on-screen display items.
*/

VOID OSD_Refresh()
{
    UINT8 items = sizeof(OsdItems) / sizeof(OsdItems[0]);
    UINT8 i;

    PushSharedMemory->NumberOfOsdItems = items;

    // Loop and draw all on screen display items
    for (i = 0; i < items; i++)
    {
        GetValues(&OsdItems[i]);

        if (!OsdItems[i].Flag //draw if no flag, could be somebody just wants to display stuff on-screen
            || PushSharedMemory->OSDFlags & OsdItems[i].Flag //if it has a flag, is it set?
            || (OsdItems[i].Threshold && OsdItems[i].Value > OsdItems[i].Threshold)) //is the item's value > it's threshold?
        {
            OsdItems[i].Color = 0xFFFFFF00;

            PushSharedMemory->OSDFlags |= OsdItems[i].Flag;

            // Check what type of formating user wants
            if (OsdItems[i].DynamicFormat)
            {
                OsdItems[i].DynamicFormat(OsdItems[i].ValueOverride ? OsdItems[i].ValueOverride : 0, OsdItems[i].Text);
            }
            else
            {
                swprintf(
                    OsdItems[i].Text,
                    20,
                    OsdItems[i].DisplayFormat,
                    OsdItems[i].ValueOverride ? OsdItems[i].ValueOverride : OsdItems[i].Value
                    );
            }

            wcscat(OsdItems[i].Text, L"\n");

            if (PushSharedMemory->Overloads & OsdItems[i].Flag)
                OsdItems[i].Color = 0xFFFF0000;
        }
    }

    if (PushOverlayInterface == OVERLAY_INTERFACE_PURE)
        memcpy(PushSharedMemory->OsdItems, OsdItems, sizeof(OsdItems));
    else if (PushOverlayInterface == OVERLAY_INTERFACE_RTSS)
        RTSS_Update(OsdItems);
}