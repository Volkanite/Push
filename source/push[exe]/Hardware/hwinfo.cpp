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
#include "CPU/intel.h"
#include "disk.h"
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
    if (!HwGpu) 
        return 0;

    return HwGpu->GetEngineClock();
}


UINT16
GetMemoryClock()
{
    if (!HwGpu) 
        return 0;

    return HwGpu->GetMemoryClock();
}


UINT8
GetGpuTemp()
{
    if (!HwGpu) 
        return 0;

    return HwGpu->GetTemperature();
}


UINT8
GetGpuLoadHardware()
{
    if (!HwGpu) 
        return 0;

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

    if (!HwGpu) 
        return 0;

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


typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef struct _PH_UINT64_DELTA
{
    UINT64 Value;
    UINT64 Delta;
} PH_UINT64_DELTA, *PPH_UINT64_DELTA;


#define PhUpdateDelta(DltMgr, NewValue) \
    ((DltMgr)->Delta = (NewValue) - (DltMgr)->Value, \
    (DltMgr)->Value = (NewValue), (DltMgr)->Delta)

FLOAT PhCpuKernelUsage;
FLOAT PhCpuUserUsage;
FLOAT *PhCpusKernelUsage;
FLOAT* PhCpusUserUsage;

PPH_UINT64_DELTA PhCpusKernelDelta;
PPH_UINT64_DELTA PhCpusUserDelta;
PPH_UINT64_DELTA PhCpusIdleDelta;

PH_UINT64_DELTA PhCpuKernelDelta;
PH_UINT64_DELTA PhCpuUserDelta;
PH_UINT64_DELTA PhCpuIdleDelta;

SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *PhCpuInformation;



FLOAT
GetCpuUsage( UINT8 Cpu );



UINT8
GetCpuLoad()
{
    return (PhCpuUserUsage + PhCpuKernelUsage) * 100;
}


FLOAT
GetCpuUsage( UINT8 Cpu )
{
    return (PhCpusUserUsage[Cpu] + PhCpusKernelUsage[Cpu]) * 100;
}


#define SUCCEEDED(hr) (((long)(hr)) >= 0)


UINT8
GetMaxCoreLoad()
{
    FLOAT usage = 0.0f;
    FLOAT maxUsage = 0.0f;
    UINT8 i = 0;

    for (i = 0; i < hardware.Processor.NumberOfCores; i++)
    {
        usage = GetCpuUsage(i);

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


VOID 
PhProcessProviderInitialization()
{
    FLOAT *usageBuffer;
    PPH_UINT64_DELTA deltaBuffer;

    NtQuerySystemInformation(
        SystemBasicInformation, 
        &PhSystemBasicInformation, 
        sizeof(SYSTEM_BASIC_INFORMATION), 
        NULL
        );

    PhCpuInformation = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*) RtlAllocateHeap(
        PushHeapHandle, 
        0, 
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * (ULONG)PhSystemBasicInformation.NumberOfProcessors
        );

    usageBuffer = (FLOAT*) RtlAllocateHeap(
        PushHeapHandle, 
        0, 
        sizeof(FLOAT) * (ULONG)PhSystemBasicInformation.NumberOfProcessors * 2
        );

    deltaBuffer = (PPH_UINT64_DELTA) RtlAllocateHeap(
        PushHeapHandle, 
        0, 
        sizeof(PH_UINT64_DELTA) * (ULONG)PhSystemBasicInformation.NumberOfProcessors * 3
        );

    PhCpusKernelUsage = usageBuffer;
    PhCpusUserUsage = PhCpusKernelUsage + (ULONG)PhSystemBasicInformation.NumberOfProcessors;

    PhCpusKernelDelta = deltaBuffer;
    PhCpusUserDelta = PhCpusKernelDelta + (ULONG)PhSystemBasicInformation.NumberOfProcessors;
    PhCpusIdleDelta = PhCpusUserDelta + (ULONG)PhSystemBasicInformation.NumberOfProcessors;

    memset(deltaBuffer, 0, sizeof(PH_UINT64_DELTA) * (ULONG)PhSystemBasicInformation.NumberOfProcessors);
}


VOID 
PhpUpdateCpuInformation()
{
    ULONG i;
    UINT64 totalTime;
    
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
    static BOOLEAN init = FALSE;

    if (!init)
    {
        PhProcessProviderInitialization();

        init = TRUE;
    }

    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation, 
        PhCpuInformation, 
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * (ULONG)PhSystemBasicInformation.NumberOfProcessors, 
        NULL
        );

    // Zero the CPU totals.
    memset(&PhCpuTotals, 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *cpuInfo = &PhCpuInformation[i];

        // KernelTime includes idle time.
        cpuInfo->KernelTime.QuadPart -= cpuInfo->IdleTime.QuadPart;
        cpuInfo->KernelTime.QuadPart += cpuInfo->DpcTime.QuadPart + cpuInfo->InterruptTime.QuadPart;

        PhCpuTotals.DpcTime.QuadPart += cpuInfo->DpcTime.QuadPart;
        PhCpuTotals.IdleTime.QuadPart += cpuInfo->IdleTime.QuadPart;
        PhCpuTotals.InterruptCount += cpuInfo->InterruptCount;
        PhCpuTotals.InterruptTime.QuadPart += cpuInfo->InterruptTime.QuadPart;
        PhCpuTotals.KernelTime.QuadPart += cpuInfo->KernelTime.QuadPart;
        PhCpuTotals.UserTime.QuadPart += cpuInfo->UserTime.QuadPart;

        PhUpdateDelta(&PhCpusKernelDelta[i], cpuInfo->KernelTime.QuadPart);
        PhUpdateDelta(&PhCpusUserDelta[i], cpuInfo->UserTime.QuadPart);
        PhUpdateDelta(&PhCpusIdleDelta[i], cpuInfo->IdleTime.QuadPart);

        totalTime = PhCpusKernelDelta[i].Delta + PhCpusUserDelta[i].Delta + PhCpusIdleDelta[i].Delta;

        if (totalTime != 0)
        {
            PhCpusKernelUsage[i] = (FLOAT)PhCpusKernelDelta[i].Delta / totalTime;
            PhCpusUserUsage[i] = (FLOAT)PhCpusUserDelta[i].Delta / totalTime;
        }
        else
        {
            PhCpusKernelUsage[i] = 0;
            PhCpusUserUsage[i] = 0;
        }
    }

    PhUpdateDelta(&PhCpuKernelDelta, PhCpuTotals.KernelTime.QuadPart);
    PhUpdateDelta(&PhCpuUserDelta, PhCpuTotals.UserTime.QuadPart);
    PhUpdateDelta(&PhCpuIdleDelta, PhCpuTotals.IdleTime.QuadPart);

    totalTime = PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta + PhCpuIdleDelta.Delta;

    if (totalTime != 0)
    {
        PhCpuKernelUsage = (FLOAT)PhCpuKernelDelta.Delta / totalTime;
        PhCpuUserUsage = (FLOAT)PhCpuUserDelta.Delta / totalTime;
    }
    else
    {
        PhCpuKernelUsage = 0;
        PhCpuUserUsage = 0;
    }
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

                R0ReadPciConfig(pciAddress, 0, (BYTE *)conf, sizeof(conf));

                if(func == 0) // Is Multi Function Device
                {
                    R0ReadPciConfig(pciAddress, 0x0E, (BYTE *)&type, sizeof(type));

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

    HwGpu = CreateGpuInterface( hardware.DisplayDevice.VendorId );

    if (HwGpu)
    {
        hardware.DisplayDevice.FrameBuffer.Total = HwGpu->GetTotalMemory();
        hardware.DisplayDevice.EngineClockMax   = HwGpu->GetMaximumEngineClock();
        hardware.DisplayDevice.MemoryClockMax   = HwGpu->GetMaximumMemoryClock();
    }

    if (IniReadBoolean(L"Settings", L"GpuUsageD3DKMT", FALSE))
        PushGpuLoadD3DKMT = TRUE;

    /*if (PushGpuLoadD3DKMT || hardware.DisplayDevice.VendorId)
        //InitializeD3DKMT();
        D3DKMTInitialize();*/

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
    PhpUpdateCpuInformation();

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
