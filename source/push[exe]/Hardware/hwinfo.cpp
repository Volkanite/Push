#include <sltypes.h>
#include <slnt.h>
#include <slntuapi.h>
#include <pushbase.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <slc.h>
#include <slmodule.h>

#include "..\push.h"
#include "..\ring0.h"
#include "..\ini.h"


#include "hwinfo.h"
<<<<<<< HEAD:source/push[exe]/Hardware/hwinfo.cpp
//#include "Nvidia/geforce.h"
//#include "ATI/radeon.h"
//#include "Intel\intel.h"
#include "CPU/intel.h"
=======
#include "Nvidia\geforce.h"
#include "ATI\radeon.h"
#include "Intel\intel.h"
>>>>>>> master:source/push[exe]/Hardware/hwinfo.c
#include "disk.h"
//#include "d3dkmt.h"
#include "GPU/d3dkmt.h"


UINT64  g_hNVPMContext;
BOOLEAN    PushGpuLoadD3DKMT = FALSE;
DWORD dwMappedMemAddr;
PUSH_HARDWARE_INFORMATION hardware;


#define REGISTER_VENDORID   0x00
#define REGISTER_CLASSCODE  0x08
#define REGISTER_BAR0       0x10
#define REGISTER_BAR2       0x18
#include "GPU\gpu.h"
CGPU* HwGpu;


UINT16
GetEngineClock()
{
    return HwGpu->GetEngineClock();
}


UINT16
GetMemoryClock()
{
    return HwGpu->GetMemoryClock();
}


UINT8
GetGpuTemp()
{
    return HwGpu->GetTemperature();
}


UINT8
GetGpuLoadHardware()
{
    return HwGpu->GetLoad();
}


UINT8
GetGpuLoad()
{
    if (PushGpuLoadD3DKMT)
        //return GetGpuLoadD3DKMT();
        return D3DKMTGetGpuUsage();
    else
        return GetGpuLoadHardware();
}


UINT8
GetCpuTemp()
{
    return GetIntelTemp();
}

UINT64
D3DKMTGetMemoryUsage();


UINT32
GetVramUsed()
{
    UINT64 total, free, used;

    total = HwGpu->GetTotalMemory();
    free = HwGpu->GetFreeMemory();
    used = total - free;

    return used / 1048576;
}


UINT8
GetVramUsage()
{
    return 100 * ( (float) hardware.DisplayDevice.FrameBuffer.Used / (float) hardware.DisplayDevice.FrameBuffer.Total );
}


UINT32
GetFrameBufferSize()
{
    return HwGpu->GetTotalMemory();
}


UINT32
GetRamUsed()
{
    SYSTEM_BASIC_PERFORMANCE_INFORMATION performanceInfo;
    UINT64 totalPageFile, availablePageFile;

    NtQuerySystemInformation(
        SystemBasicPerformanceInformation,
        &performanceInfo,
        sizeof(SYSTEM_BASIC_PERFORMANCE_INFORMATION),
        NULL
        );

    totalPageFile = performanceInfo.CommitLimit;
    totalPageFile *= PushPageSize;

    availablePageFile = performanceInfo.CommitLimit - performanceInfo.CommittedPages;
    availablePageFile *= PushPageSize;

    return (totalPageFile - availablePageFile) / 1048576;
}


UINT8
GetRamUsage()
{
    return 100 * ( (float) hardware.Memory.Used / (float) hardware.Memory.Total);
}


UINT8
GetCpuLoad()
{
    ULARGE_INTEGER idle, kernel, user;
    static ULARGE_INTEGER  idle_old, kernel_old, user_old;
    INT32 usage;

    GetSystemTimes( (FILETIME *) &idle, (FILETIME *) &kernel, (FILETIME *) &user);

    usage = (((((kernel.QuadPart - kernel_old.QuadPart) + (user.QuadPart - user_old.QuadPart)) - (idle.QuadPart - idle_old.QuadPart)) * (100)) / ((kernel.QuadPart - kernel_old.QuadPart) + (user.QuadPart    - user_old.QuadPart)));

    idle_old.QuadPart   = idle.QuadPart;
    user_old.QuadPart   = user.QuadPart;
    kernel_old.QuadPart = kernel.QuadPart;

    return usage;
}

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER   IdleTime;
    LARGE_INTEGER   KernelTime;
    LARGE_INTEGER   UserTime;
    LARGE_INTEGER   Reserved1[2];
    ULONG           Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;


#define SUCCEEDED(hr) (((long)(hr)) >= 0)


FLOAT
GetCpuCoreUsage( UINT8 coreNumber )
{
    LARGE_INTEGER perfCounter;
    CORE_LIST *coreListEntry = &hardware.Processor.coreList;

    while (coreListEntry->number != coreNumber)
    {
        coreListEntry = coreListEntry->nextEntry;
    }

    if (!PushSharedMemory->performanceFrequency)
        //init high-resolution timer if it is not initialized yet
        NtQueryPerformanceCounter(&perfCounter, (LARGE_INTEGER*) &PushSharedMemory->performanceFrequency );

    if (PushSharedMemory->performanceFrequency)
        //we can perform calcualtion only if high-resolution timer is available
    {
        NtQueryPerformanceCounter(&perfCounter, NULL);
            //query high-resolution time counter

        if (perfCounter.QuadPart - coreListEntry->perfCounter.QuadPart >= PushSharedMemory->performanceFrequency)
            //update usage once per second
        {
            UINT32 size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * hardware.Processor.NumberOfCores;

            SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *info = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*) RtlAllocateHeap(PushHeapHandle, 0, size);

            if (SUCCEEDED(NtQuerySystemInformation(SystemProcessorPerformanceInformation, info, size, NULL)))
                //query CPU usage
            {
                if (coreListEntry->idleTime.QuadPart)
                    //ensure that this function was already called at least once
                    //and we have the previous idle time value
                {
                    coreListEntry->usage = 100.0f - 0.00001f * (info[coreListEntry->number].IdleTime.QuadPart - coreListEntry->idleTime.QuadPart) * PushSharedMemory->performanceFrequency / (perfCounter.QuadPart - coreListEntry->perfCounter.QuadPart);
                        //calculate new CPU usage value by estimating amount of time
                        //CPU was in idle during the last second

                    //clip calculated CPU usage to [0-100] range to filter calculation non-ideality

                    if (coreListEntry->usage < 0.0f)
                        coreListEntry->usage = 0.0f;

                    if (coreListEntry->usage > 100.0f)
                        coreListEntry->usage = 100.0f;

                }

                coreListEntry->idleTime = info[coreListEntry->number].IdleTime;
                    //save new idle time for specified CPU
                coreListEntry->perfCounter = perfCounter;
                    //save new performance counter for specified CPU

                //SlFree(info);
                RtlFreeHeap(PushHeapHandle, 0, info);
            }
        }
    }

    return coreListEntry->usage;
}


UINT8
GetMaxCoreLoad()
{
    FLOAT usage = 0.0f;
    FLOAT maxUsage = 0.0f;
    UINT8 i = 0;

    for (i = 0; i < hardware.Processor.NumberOfCores; i++)
    {
        usage = GetCpuCoreUsage(i);

        if (usage > maxUsage)
        {
            maxUsage = usage;
        }
    }

    return maxUsage;
}


UINT32
GetDiskReadWriteRate()
{
    return DiskGetBytesDelta();
}


UINT16
GetDiskResponseTime()
{
    return DiskGetResponseTime();
}


VOID
HwForceMaxClocks()
{
    HwGpu->ForceMaximumClocks();
}


DWORD GetGpuAddress()
{
    UINT8   bus             = 0;
    UINT8   dev             = 0;
    UINT8   func            = 0;
    DWORD   pciAddress      = 0xFFFFFFFF;
    DWORD   conf[3]         = {0};
    BOOLEAN    multiFuncFlag   = FALSE;
    BYTE    type            = 0;

    for(bus = 0; bus <= 255; bus++)
    {
        for(dev = 0; dev < 31; dev++)
        {
            multiFuncFlag = FALSE;

            for(func = 0; func < 7; func++)
            {
                if(multiFuncFlag == FALSE && func > 0)
                {
                    break;
                }

                pciAddress = PciBusDevFunc(bus, dev, func);

                if (!R0ReadPciConfig(pciAddress, 0, (BYTE *)conf, sizeof(conf)))
                    return NULL;

                if(func == 0) // Is Multi Function Device
                {
                    if (!R0ReadPciConfig(pciAddress, 0x0E, (BYTE *)&type, sizeof(type)))
                        return NULL;

                    if(type & 0x80)
                    {
                        multiFuncFlag = TRUE;
                    }

                }

                if((conf[2] & 0xFFFFFF00) ==
                        (((DWORD)0x03 << 24) |
                        ((DWORD)0x00 << 16) |
                        ((DWORD)0x00 << 8))
                    )
                {
                    return pciAddress;
                }

            }
        }
    }
    return 0xFFFFFFFF;
}


VOID* HwMmio;
DWORD
ReadGpuRegister( DWORD Address )
{
    DWORD* val=0;
    DWORD address;
    DWORD ret;

    address = ((DWORD)HwMmio) + Address;

    val = (DWORD*)address;
    ret = *val;
    return ret;
}


DWORD
GetBarAddress( DWORD Bar )
{
    DWORD barAddress;

    R0ReadPciConfig(
        hardware.DisplayDevice.pciAddress,
        Bar,
        (BYTE *)&barAddress,
        sizeof(barAddress)
        );

    // 16-Byte alligned address;
    return (barAddress & 0x0fffffff0);
}


UINT32
GetBarSize(
    DWORD BarAddress
    )
{
   unsigned pos = 0;

   while (!(BarAddress & 1))
   {
      BarAddress >>= 1;
      ++pos;
   }

    return 1 << (pos - 1);
}


VOID
InitGpuHardware()
{
    DWORD bar;

    hardware.DisplayDevice.pciAddress = GetGpuAddress();

    R0ReadPciConfig(
        hardware.DisplayDevice.pciAddress,
        REGISTER_VENDORID,
        (BYTE *) &hardware.DisplayDevice.VendorId,
        sizeof(hardware.DisplayDevice.VendorId)
        );

    if (hardware.DisplayDevice.VendorId == NVIDIA)
        bar = REGISTER_BAR0;
    else if (hardware.DisplayDevice.VendorId == AMD)
        bar = REGISTER_BAR2;
    else
        return;

    hardware.DisplayDevice.BarAddress = GetBarAddress(bar);

    HwMmio = R0MapPhysicalMemory(
                hardware.DisplayDevice.BarAddress,
                GetBarSize(hardware.DisplayDevice.BarAddress)
                );
}


VOID
GetHardwareInfo()
{
    SYSTEM_BASIC_INFORMATION basicInfo;
    SYSTEM_BASIC_PERFORMANCE_INFORMATION performanceInfo;
    int i = 0;
    UINT64 totalPageFile;
    CORE_LIST *coreListEntry;

    InitGpuHardware();

    hardware.DisplayDevice.FrameBuffer.Total = HwGpu->GetTotalMemory();
    hardware.DisplayDevice.EngineClockMax   = HwGpu->GetMaximumEngineClock();
    hardware.DisplayDevice.MemoryClockMax   = HwGpu->GetMaximumMemoryClock();

    if (IniReadBoolean(L"Settings", L"GpuUsageD3DKMT", FALSE))
    {
        PushGpuLoadD3DKMT = TRUE;
    }

    if (PushGpuLoadD3DKMT || hardware.DisplayDevice.VendorId)
        //InitializeD3DKMT();
        D3DKMTInitialize();

    // Get the number of processors in the system
    NtQuerySystemInformation(
        SystemBasicInformation,
        &basicInfo,
        sizeof(SYSTEM_BASIC_INFORMATION),
        0
        );

    hardware.Processor.NumberOfCores = basicInfo.NumberOfProcessors;
    PushPageSize = basicInfo.PageSize;

    coreListEntry = &hardware.Processor.coreList;

    for (i = 0; i < hardware.Processor.NumberOfCores; i++)
    {
        coreListEntry->nextEntry = (CORE_LIST*)RtlAllocateHeap(
                                    PushHeapHandle,
                                    0,
                                    sizeof(CORE_LIST)
                                    );

        coreListEntry->number                   = i;
        coreListEntry->idleTime.QuadPart        = 0;
        coreListEntry->perfCounter.QuadPart     = 0;
        coreListEntry->usage                    = 0.0f;

        coreListEntry = coreListEntry->nextEntry;
    }

    //InitGeForce(hardware.DisplayDevice.coreFamily);

    //initialize memory info
    NtQuerySystemInformation(
        SystemBasicPerformanceInformation,
        &performanceInfo,
        sizeof(SYSTEM_BASIC_PERFORMANCE_INFORMATION),
        NULL
        );

    totalPageFile = performanceInfo.CommitLimit;
    totalPageFile *= PushPageSize;

    hardware.Memory.Total = (totalPageFile >> 20);

    // Start disk monitoring;
    DiskStartMonitoring();
}


VOID
RefreshHardwareInfo()
{
    hardware.Processor.Load                 = GetCpuLoad();
    hardware.Processor.MaxCoreUsage         = GetMaxCoreLoad();
    hardware.Processor.Temperature          = GetCpuTemp();
    hardware.DisplayDevice.Load             = GetGpuLoad();
    hardware.DisplayDevice.EngineClock      = GetEngineClock();
    hardware.DisplayDevice.MemoryClock      = GetMemoryClock();
    hardware.DisplayDevice.Temperature      = GetGpuTemp();
    hardware.DisplayDevice.FrameBuffer.Used = GetVramUsed();
    hardware.DisplayDevice.FrameBuffer.Load = GetVramUsage();
    hardware.Memory.Used                    = GetRamUsed();
    hardware.Memory.Load                    = GetRamUsage();
    hardware.Disk.ReadWriteRate             = GetDiskReadWriteRate();
    hardware.Disk.ResponseTime              = GetDiskResponseTime();

    // We actually get some information from other sources
    hardware.Processor.MaxThreadUsage = PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage;


    /*if (PushSharedMemory->ThreadMonitorOSD)
    {
        hardware.Processor.MaxThreadUsage = GetMaxThreadUsage(
                                                PushSharedMemory->GameProcessID
                                                );

        if(g_ThreadOptimization && !g_ThreadListLock)
        {
            RunThreadOptimizeRoutine();
        }
    }*/
}
