#include <sl.h>
#include <time.h>
#include <push.h>
#include <hardware.h>

#include "rtss.h"

int NumberOfItems;

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
    OSD_ITEM *osdItem;

    osdItem = (OSD_ITEM*)OsdItems;

    // Loop and draw all on screen display items
    for (i = 0; i < NumberOfItems; i++, osdItem++)
    {
        //GetValues(&OsdItems[i]);

        switch (osdItem->ValueSize)
        {
        case sizeof(UINT8) :
            osdItem->Value = osdItem->ValueSourcePtr ? *(UINT8*)osdItem->ValueSourcePtr : 0;
            break;
        case sizeof(UINT16) :
            osdItem->Value = osdItem->ValueSourcePtr ? *(UINT16*)osdItem->ValueSourcePtr : 0;
            break;
        case sizeof(UINT32) :
            osdItem->Value = osdItem->ValueSourcePtr ? *(UINT32*)osdItem->ValueSourcePtr : 0;
            break;
        }

        osdItem->Value2 = osdItem->ValueSource2Ptr ? *(UINT32*)osdItem->ValueSource2Ptr : 0;

        //do some over-rides
        if (osdItem->Flag == OSD_CPU_LOAD)
        {
            switch (CPUStrap)
            {
            case CPU_CALC_twc:
                osdItem->Value = PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage;
                break;
            case CPU_CALC_t:
                osdItem->Value = PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage;
                break;
            case CPU_CALC_o:
                osdItem->Value = PushSharedMemory->HarwareInformation.Processor.Load;
                break;
            case CPU_CALC_c:
                osdItem->Value = PushSharedMemory->HarwareInformation.Processor.MaxCoreUsage;
                break;
            }
        }

        //calculate average
        osdItem->Samples++;
        osdItem->RunningDelta += osdItem->Value;
        osdItem->ValueAvg = osdItem->RunningDelta / osdItem->Samples;

        if (osdItem->Value > osdItem->HighestDelta)
            osdItem->HighestDelta = osdItem->Value;

        //set default color (yellow)
        osdItem->Color = 0xFFFFFF00;

        if (!osdItem->Flag //draw if no flag, could be somebody just wants to display stuff on-screen
            || PushSharedMemory->OSDFlags & osdItem->Flag //if it has a flag, is it set?
            || PushSharedMemory->Overloads & osdItem->Flag //item signifies performance issue?
            || (osdItem->Threshold && osdItem->Value > osdItem->Threshold) //is the item's value > it's threshold?
            || (osdItem->Triggered && osdItem->Value > osdItem->HighestDelta) //is the item's value > it's maximal?
            || (osdItem->Queue && osdItem->Value > osdItem->ValueAvg)) //add's a more dynamic touch ;)
        {
            osdItem->Triggered = TRUE;
            osdItem->Queue = TRUE;

            if (osdItem->DisplayFormatPtr || osdItem->DynamicFormatPtr)
            {
                if (osdItem->DynamicFormatPtr)
                {
                    OSD_DYNAMIC_FORMAT dynamicFormat;

                    dynamicFormat = (OSD_DYNAMIC_FORMAT) osdItem->DynamicFormatPtr;

                    dynamicFormat(osdItem->Value2 ? osdItem->Value2 : 0, osdItem->Text);

                    String_Concatenate(osdItem->Text, L"\n");
                }
                else if (osdItem->DisplayFormatPtr)
                {
                    wchar_t buffer[260];
                    int charactersWritten;

                    charactersWritten = String_Format(
                        buffer,
                        260,
                        (WCHAR*) osdItem->DisplayFormatPtr,
                        osdItem->Value2 ? osdItem->Value2 : osdItem->Value
                        );

                    String_CopyN(osdItem->Text, buffer, 20);

                    osdItem->Text[18] = L'\0';
                    osdItem->Text[19] = L'\n';

                    if (charactersWritten > 18)
                    {
                        Log(L"OSD_ITEM::Text buffer is too small for %s", buffer);
                    }
                }
            }

            if (PushSharedMemory->Overloads & osdItem->Flag)
            {
                osdItem->Color = 0xFFFF0000;
            } 
        }
        else
        {
            osdItem->Queue = FALSE;
        }
    }

    if (PushOverlayInterface == OVERLAY_INTERFACE_PURE)
    {
        Memory_Copy(PushSharedMemory->OsdItems, OsdItems, sizeof(OSD_ITEM)* NumberOfItems);
    }
    else if (PushOverlayInterface == OVERLAY_INTERFACE_RTSS)
    {
        RTSS_Update(OsdItems);
    }
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

    OsdItems[NumberOfItems].Flag = Flag;
    OsdItems[NumberOfItems].DisplayFormatPtr = DisplayFormat;
    OsdItems[NumberOfItems].ValueSourcePtr = ValueSource;
    OsdItems[NumberOfItems].ValueSize = ValueSize;
    OsdItems[NumberOfItems].ValueSource2Ptr = ValueSource2;
    OsdItems[NumberOfItems].Threshold = Threshold;
    OsdItems[NumberOfItems].DynamicFormatPtr = DynamicFormat;

    String_CopyN(OsdItems[NumberOfItems].Description, Description, 40);

    NumberOfItems++;
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
    OSD_AddItem(OSD_REFRESH_RATE, L"Resfresh Rate", L"MON : %i Hz", &hardware->Display.RefreshRate, sizeof(UINT8), NULL, 0, NULL);

    PushSharedMemory->NumberOfOsdItems = NumberOfItems;

    return sizeof(OSD_ITEM) * NumberOfItems;
}