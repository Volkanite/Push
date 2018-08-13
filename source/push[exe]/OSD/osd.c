#include <sl.h>
#include <time.h>
#include <push.h>
#include <hardware.h>

#include "rtss.h"

int items;
typedef enum CPU_CALC_INDEX
{
    CPU_CALC_twc,
    CPU_CALC_t,
    CPU_CALC_o,
    CPU_CALC_c

} CPU_CALC_INDEX;
CPU_CALC_INDEX CPUStrap = CPU_CALC_twc;/*CPU_CALC_o;*/

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


OSD_ITEM* OsdItems;


VOID FormatTime( UINT32 Value, WCHAR* Buffer )
{
    Push_FormatTime(Buffer);
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

    String_Format(Buffer, 20, format, rate);
}


/**
* Refreshes all on-screen display items.
*/

VOID OSD_Refresh()
{
    UINT8 i;

    PushSharedMemory->NumberOfOsdItems = items;

    // Loop and draw all on screen display items
    for (i = 0; i < items; i++)
    {
        //GetValues(&OsdItems[i]);

        switch (OsdItems[i].ValueSize)
        {
        case sizeof(UINT8) :
            OsdItems[i].Value = OsdItems[i].ValueSourcePtr ? *(UINT8*)OsdItems[i].ValueSourcePtr : 0;
            break;
        case sizeof(UINT16) :
            OsdItems[i].Value = OsdItems[i].ValueSourcePtr ? *(UINT16*)OsdItems[i].ValueSourcePtr : 0;
            break;
        case sizeof(UINT32) :
            OsdItems[i].Value = OsdItems[i].ValueSourcePtr ? *(UINT32*)OsdItems[i].ValueSourcePtr : 0;
            break;
        }

        OsdItems[i].Value2 = OsdItems[i].ValueSource2Ptr ? *(UINT32*)OsdItems[i].ValueSource2Ptr : 0;

        //do some over-rides
        if (OsdItems[i].Flag == OSD_CPU_LOAD)
        {
            switch (CPUStrap)
            {
            case CPU_CALC_twc:
                OsdItems[i].Value = PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage;
                break;
            case CPU_CALC_t:
                OsdItems[i].Value = PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage;
                break;
            case CPU_CALC_o:
                OsdItems[i].Value = PushSharedMemory->HarwareInformation.Processor.Load;
                break;
            case CPU_CALC_c:
                OsdItems[i].Value = PushSharedMemory->HarwareInformation.Processor.MaxCoreUsage;
                break;
            }

            //Log(L"CPU : %i %%, avg: %i %%", OsdItems[i].Value, OsdItems[i].ValueAvg);
        }

        //calculate average
        OsdItems[i].Samples++;
        OsdItems[i].SampleHigh += OsdItems[i].Value;
        OsdItems[i].ValueAvg = OsdItems[i].SampleHigh / OsdItems[i].Samples;

        //set default color (yellow)
        OsdItems[i].Color = 0xFFFFFF00;

        if (!OsdItems[i].Flag //draw if no flag, could be somebody just wants to display stuff on-screen
            || PushSharedMemory->OSDFlags & OsdItems[i].Flag //if it has a flag, is it set?
            || PushSharedMemory->Overloads & OsdItems[i].Flag //item signifies performance issue?
            || (OsdItems[i].Threshold && OsdItems[i].Value > OsdItems[i].Threshold) //is the item's value > it's threshold?
            || (OsdItems[i].Triggered && OsdItems[i].Value > OsdItems[i].ValueAvg)) //add's a more dynamic touch ;)
        {
            OsdItems[i].Triggered = TRUE;
            OsdItems[i].Queue = TRUE;

            if (OsdItems[i].DisplayFormatPtr || OsdItems[i].DynamicFormatPtr)
            {
                if (OsdItems[i].DynamicFormatPtr)
                {
                    OSD_DYNAMIC_FORMAT dynamicFormat;

                    dynamicFormat = (OSD_DYNAMIC_FORMAT) OsdItems[i].DynamicFormatPtr;

                    dynamicFormat(OsdItems[i].Value2 ? OsdItems[i].Value2 : 0, OsdItems[i].Text);

                    String_Concatenate(OsdItems[i].Text, L"\n");
                }
                else if (OsdItems[i].DisplayFormatPtr)
                {
                    wchar_t buffer[260];
                    int charactersWritten;

                    charactersWritten = String_Format(
                        buffer,
                        260,
                        (WCHAR*) OsdItems[i].DisplayFormatPtr,
                        OsdItems[i].Value2 ? OsdItems[i].Value2 : OsdItems[i].Value
                        );

                    String_CopyN(OsdItems[i].Text, buffer, 20);

                    OsdItems[i].Text[18] = L'\0';
                    OsdItems[i].Text[19] = L'\n';

                    if (charactersWritten > 18)
                    {
                        Log(L"OSD_ITEM::Text buffer is too small for %s", buffer);
                    }
                }
            }

            if (PushSharedMemory->Overloads & OsdItems[i].Flag)
            {
                OsdItems[i].Color = 0xFFFF0000;
            } 
        }
        else
        {
            OsdItems[i].Queue = FALSE;
        }
    }

    if (PushOverlayInterface == OVERLAY_INTERFACE_PURE)
        memcpy(PushSharedMemory->OsdItems, OsdItems, sizeof(OSD_ITEM) * items);
    else if (PushOverlayInterface == OVERLAY_INTERFACE_RTSS)
        RTSS_Update(OsdItems);
}


VOID OSD_AddItem(
    DWORD Flag,
    WCHAR* Description,
    WCHAR* DisplayFormat,
    VOID* ValueSource,
    UINT8 ValueSize,
    UINT32* ValueSource2,
    UINT16 Threshold,
    OSD_DYNAMIC_FORMAT DynamicFormat
    )
{
    if (!OsdItems)
        OsdItems = Memory_AllocateEx(sizeof(OSD_ITEM) * 21, HEAP_ZERO_MEMORY);

    OsdItems[items].Flag = Flag;
    OsdItems[items].DisplayFormatPtr = DisplayFormat;
    OsdItems[items].ValueSourcePtr = ValueSource;
    OsdItems[items].ValueSize = ValueSize;
    OsdItems[items].ValueSource2Ptr = ValueSource2;
    OsdItems[items].Threshold = Threshold;
    OsdItems[items].DynamicFormatPtr = DynamicFormat;

    String_CopyN(OsdItems[items].Description, Description, 40);

    items++;
}


UINT32 OSD_GetSize()
{
    return sizeof(OSD_ITEM) * 21;
}


UINT32 OSD_Initialize()
{
    PUSH_HARDWARE_INFORMATION* hardware = &PushSharedMemory->HarwareInformation;

    OSD_AddItem(OSD_GPU_LOAD, L"GPU Core utilization", L"GPU : %i %%", &hardware->DisplayDevice.Load, sizeof(UINT8), NULL, 90, NULL);
    OSD_AddItem(OSD_GPU_TEMP, L"GPU Core temperature", L"GPU : %i °C", &hardware->DisplayDevice.Temperature, sizeof(UINT8), NULL, 75, NULL);
    OSD_AddItem(OSD_GPU_E_CLK, L"GPU Engine Clock", L"GPU-e : %i MHz", &hardware->DisplayDevice.EngineClock, sizeof(UINT32), NULL, 0, NULL);
    OSD_AddItem(OSD_GPU_M_CLK, L"GPU Memory Clock", L"GPU-m : %i MHz", &hardware->DisplayDevice.MemoryClock, sizeof(UINT32), NULL, 0, NULL);
    OSD_AddItem(OSD_GPU_VOLTAGE, L"GPU Voltage", L"GPU : %i mV", &hardware->DisplayDevice.Voltage, sizeof(UINT32), NULL, 0, NULL);
    OSD_AddItem(OSD_GPU_FAN_RPM, L"GPU Fan Speed", L"GPU : %i RPM", &hardware->DisplayDevice.FanSpeed, sizeof(UINT32), NULL, 0, NULL);
    OSD_AddItem(OSD_GPU_FAN_DC, L"GPU Fan Duty Cycle", L"GPU : %i %%", &hardware->DisplayDevice.FanDutyCycle, sizeof(UINT8), NULL, 90, NULL);
    OSD_AddItem(OSD_GPU_VRAM, L"GPU VRAM usage", L"GPU : %i MB", &hardware->DisplayDevice.FrameBuffer.Load, sizeof(UINT8), &hardware->DisplayDevice.FrameBuffer.Used, 90, NULL);
    OSD_AddItem(OSD_RAM, L"RAM usage", L"RAM : %i MB", &hardware->Memory.Load, sizeof(UINT8), &hardware->Memory.Used, 90, NULL);
    OSD_AddItem(OSD_CPU_SPEED, L"CPU Speed", L"CPU : %i MHz", &hardware->Processor.MhzCurrent, sizeof(UINT16), NULL, 0, NULL);
    OSD_AddItem(OSD_CPU_TEMP, L"CPU temperature", L"CPU : %i °C", &hardware->Processor.Temperature, sizeof(UINT8), NULL, 75, NULL);
    OSD_AddItem(OSD_CPU_LOAD, L"CPU utilization", L"CPU : %i %%", &hardware->Processor.Load, sizeof(UINT8), NULL, 95, NULL);
    //OSD_AddItem(OSD_MCU, L"Max core usage", L"CPU-c : %i %%", &hardware->Processor.MaxCoreUsage, sizeof(UINT8), NULL, 0, NULL);
    //OSD_AddItem(OSD_MTU, L"Max thread usage", L"CPU-t : %i %%", &hardware->Processor.MaxThreadUsage, sizeof(UINT8), NULL, 0, NULL);
    OSD_AddItem(OSD_DISK_RWRATE, L"Disk read-write rate", NULL, NULL, sizeof(UINT8), NULL, 0, FormatDiskReadWriteRate);
    OSD_AddItem(OSD_DISK_RESPONSE, L"Disk response time", L"DSK : %i ms", NULL, sizeof(UINT8), NULL, 4000, NULL);
    OSD_AddItem(OSD_TIME, L"Time", NULL, NULL, sizeof(UINT8), NULL, 0, FormatTime);
    OSD_AddItem(OSD_BUFFERS, L"Frame Buffer count", NULL, NULL, sizeof(UINT8), NULL, 0, NULL);
    OSD_AddItem(OSD_RESOLUTION, L"Resolution", NULL, NULL, sizeof(UINT8), 0, 0, NULL);
    OSD_AddItem(OSD_REFRESH_RATE, L"Resfresh Rate", L"MON : %i Hz", &hardware->Display.RefreshRate, sizeof(UINT8), NULL, 0, NULL);

    return sizeof(OSD_ITEM) * items;
}