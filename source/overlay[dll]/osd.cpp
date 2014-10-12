#include <Windows.h>
#include <time.h>
#include <stdio.h>

#include "overlay.h"
#include <OvRender.h>


PUSH_HARDWARE_INFORMATION OsdHardwareInformation;


VOID FormatDiskReadWriteRate(
    UINT32 Value,
    WCHAR* Buffer
    );

VOID FormatTime(
    UINT32 Value,
    WCHAR* Buffer
    );

typedef VOID (*OSD_DYNAMIC_FORMAT)(
    UINT32 Value,
    WCHAR* Buffer
    );


typedef struct _OSD_ITEM
{
    DWORD Flag;
    UINT8 Threshold;
    WCHAR* DisplayFormat;

    //Must be at least 8 bits or you might get garbage,
    //More than 8 bits and you will get incorrect readings.
    UINT8* Value;

    //This will be displayed instead of the first, Must be at least 32 bits or you might get garbage,
    //More than 32 bits and you will get incorrect readings.
    UINT32* ValueOverride;

    //For when formatting must happen at runtime
    OSD_DYNAMIC_FORMAT DynamicFormat;
}OSD_ITEM;


OSD_ITEM OsdItems[] = {
    {OSD_GPU_LOAD,      90, L"GPU : %i %%",     &OsdHardwareInformation.DisplayDevice.Load},
    {OSD_GPU_TEMP,      75, L"GPU : %i °C",     &OsdHardwareInformation.DisplayDevice.Temperature},
    {OSD_GPU_E_CLK,     0,  L"GPU : %i MHz",    NULL, &OsdHardwareInformation.DisplayDevice.EngineClock},
    {OSD_GPU_M_CLK,     0,  L"GPU : %i MHz",    NULL, &OsdHardwareInformation.DisplayDevice.MemoryClock},
    {OSD_GPU_VRAM,      90, L"VRAM : %i MB",    &OsdHardwareInformation.DisplayDevice.FrameBuffer.Load, &OsdHardwareInformation.DisplayDevice.FrameBuffer.Used},
    {OSD_RAM,           90, L"RAM : %i MB",     &OsdHardwareInformation.Memory.Load, &OsdHardwareInformation.Memory.Used},
    {OSD_CPU_LOAD,      95, L"CPU : %i %%",     &OsdHardwareInformation.Processor.Load},
    {OSD_CPU_TEMP,      75, L"CPU : %i °C",     &OsdHardwareInformation.Processor.Temperature},
    {OSD_MCU,           0,  L"MCU : %i %%",     &OsdHardwareInformation.Processor.MaxCoreUsage},
    {OSD_MTU,           0,  L"MTU : %i %%",     &OsdHardwareInformation.Processor.MaxThreadUsage},
    {OSD_DISK_RWRATE,   0,  NULL,               NULL, &OsdHardwareInformation.Disk.ReadWriteRate, FormatDiskReadWriteRate},
    {OSD_DISK_RESPONSE, 0,  L"DSK : %i ms",     NULL, &OsdHardwareInformation.Disk.ResponseTime},
    {OSD_TIME,          0,  NULL,               NULL, NULL, FormatTime},
    {OSD_BUFFERS,       0,  L"Buffers : %i",    &PushFrameBufferCount},
    {OSD_FPS,           0,  L"%i",              &PushStableFrameRate, &PushFrameRate}
};


VOID FormatTime(
    UINT32 Value,
    WCHAR* Buffer
    )
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );

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


VOID
OsdRefresh( OvOverlay* Overlay )
{
    UINT8 items = sizeof(OsdItems) / sizeof(OsdItems[0]);
    UINT8 i;
    WCHAR buffer[20];

    // Update local hardware information
    OsdHardwareInformation = PushSharedMemory->HarwareInformation;

    // Loop and draw all on screen display items
    for (i = 0; i < items; i++)
    {
        if (!OsdItems[i].Flag //draw if no flag, could be somebody just wants to display stuff on-screen
            || PushSharedMemory->OSDFlags & OsdItems[i].Flag //if it has a flag, is it set?
            || (OsdItems[i].Threshold && *OsdItems[i].Value > OsdItems[i].Threshold)) //is the item's value > it's threshold?
        {
            //BOOLEAN warning = FALSE;
            DWORD color = 0xFFFFFF00;

            PushSharedMemory->OSDFlags |= OsdItems[i].Flag;

            // Check what type of formating user wants
            if (OsdItems[i].DynamicFormat)
            {
                OsdItems[i].DynamicFormat( OsdItems[i].ValueOverride ? *OsdItems[i].ValueOverride : 0, buffer );
            }
            else
            {
                swprintf(
                    buffer,
                    ARRAYSIZE(buffer),
                    OsdItems[i].DisplayFormat,
                    OsdItems[i].ValueOverride ? *OsdItems[i].ValueOverride : *OsdItems[i].Value
                    );
            }

            wcscat(buffer, L"\n");

            if (PushSharedMemory->Overloads & OsdItems[i].Flag)
                //warning = TRUE;
                color = 0xFFFF0000;

            Overlay->DrawText(buffer, color);
        }
    }
}
