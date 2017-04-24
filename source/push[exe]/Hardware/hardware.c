#include <sl.h>
#include <string.h>
#include <push.h>
#include <ring0.h>

#include "CPU\cpu.h"
#include "disk.h"
#include "hardware.h"
#include "GPU\gpu.h"


UINT64                      g_hNVPMContext;
BOOLEAN                     PushGpuLoadD3DKMT = FALSE;
DWORD                       dwMappedMemAddr;
//PUSH_HARDWARE_INFORMATION   hardware;
SYSTEM_BASIC_INFORMATION    HwInfoSystemBasicInformation;


UINT8 GetCpuTemp()
{
    return CPU_GetTemperature();
}


UINT64
D3DKMTGetMemoryUsage();


UINT32 GetFrameBufferSize()
{
    return GPU_GetTotalMemory();
}


//in megabytes
UINT32 GetRamUsed()
{
    SYSTEM_BASIC_PERFORMANCE_INFORMATION performanceInfo;
    UINT64 committedPages; //force allmul() on x86

    NtQuerySystemInformation(
        SystemBasicPerformanceInformation, 
        &performanceInfo, 
        sizeof(SYSTEM_BASIC_PERFORMANCE_INFORMATION), 
        NULL
        );

    committedPages = performanceInfo.CommittedPages;

    return (committedPages * HwInfoSystemBasicInformation.PageSize) / 1048576; //byte => megabytes
}


//percentage (non-float)
UINT8 GetRamUsage()
{
    return 100 * ((float)PushSharedMemory->HarwareInformation.Memory.Used / (float)PushSharedMemory->HarwareInformation.Memory.Total);
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

    for (i = 0; i < PushSharedMemory->HarwareInformation.Processor.NumberOfCores; i++)
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


UINT16 GetDiskResponseTime( UINT32 ProcessId )
{
    return DiskGetResponseTime(ProcessId);
}


VOID Hardware_ForceMaxClocks()
{
    GPU_ForceMaximumClocks();
}


VOID PhProcessProviderInitialization()
{
    FLOAT *usageBuffer;
    PPH_UINT64_DELTA deltaBuffer;

    PhCpuInformation = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*) RtlAllocateHeap(
        PushHeapHandle, 
        0, 
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors
        );

    usageBuffer = (FLOAT*) RtlAllocateHeap(
        PushHeapHandle, 
        0, 
        sizeof(FLOAT) * (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors * 2
        );

    deltaBuffer = (PPH_UINT64_DELTA) RtlAllocateHeap(
        PushHeapHandle, 
        0, 
        sizeof(PH_UINT64_DELTA) * (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors * 3
        );

    PhCpusKernelUsage = usageBuffer;
    PhCpusUserUsage = PhCpusKernelUsage + (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors;

    PhCpusKernelDelta = deltaBuffer;
    PhCpusUserDelta = PhCpusKernelDelta + (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors;
    PhCpusIdleDelta = PhCpusUserDelta + (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors;

    memset(deltaBuffer, 0, sizeof(PH_UINT64_DELTA) * (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors);
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
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors,
        NULL
        );

    // Zero the CPU totals.
    memset(&PhCpuTotals, 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    for (i = 0; i < (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors; i++)
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


DWORD FindPciDeviceByClass( BYTE baseClass, BYTE subClass, BYTE programIf, BYTE index )
{
    if(R0DriverHandle == INVALID_HANDLE_VALUE)
    {
        return 0xFFFFFFFF;
    }

    DWORD bus = 0, dev = 0, func = 0;
    DWORD count = 0;
    DWORD pciAddress = 0xFFFFFFFF;
    DWORD conf[3] = {0};
    DWORD error = 0;    
    BOOLEAN multiFuncFlag = FALSE;
    BYTE type = 0;
    count = 0;

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
                if (R0ReadPciConfig(pciAddress, 0, (BYTE*)conf, sizeof(conf)))
                {
                    if(func == 0) // Is Multi Function Device
                    { 
                        if (R0ReadPciConfig(pciAddress, 0x0E, (BYTE*)&type, sizeof(type)))
                        {
                            if(type & 0x80)
                            {
                                multiFuncFlag = TRUE;
                            }
                        }
                    }
                    if((conf[2] & 0xFFFFFF00) == 
                            (((DWORD)baseClass << 24) |
                            ((DWORD)subClass << 16) |
                            ((DWORD)programIf << 8))
                        )
                    {
                        if (count == index)
                        {
                            return pciAddress;
                        }

                        count++;
                        continue;
                    }
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
        PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress,
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


VOID InitGpuHardware()
{
    DWORD bar;

    PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress = FindPciDeviceByClass(0x03, 0x00, 0x00, 0);

    R0ReadPciConfig(
        PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress,
        REGISTER_VENDORID,
        (BYTE *)&PushSharedMemory->HarwareInformation.DisplayDevice.VendorId,
        sizeof(PushSharedMemory->HarwareInformation.DisplayDevice.VendorId)
        );

    if (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId == NVIDIA)
        bar = REGISTER_BAR0;
    else if (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId == AMD)
        bar = REGISTER_BAR2;
    else
        return;

    PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress = GetBarAddress(bar);

    HwMmio = R0MapPhysicalMemory(
        PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress,
        GetBarSize(PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress)
                );
}


VOID GetHardwareInfo()
{
    int i = 0;
    CORE_LIST *coreListEntry;
    
    //use 64 bits to force allmul() on 32 bit builds
    UINT64 physicalPages;
    UINT64 pageSize;

    InitGpuHardware();
    GPU_Initialize(PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress);

    PushSharedMemory->HarwareInformation.DisplayDevice.FrameBuffer.Total = GPU_GetTotalMemory();
    PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax = GPU_GetMaximumEngineClock();
    PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax = GPU_GetMaximumMemoryClock();
    PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax = GPU_GetMaximumVoltage();

    if (SlIniReadBoolean(L"Settings", L"GpuUsageD3DKMT", FALSE, L".\\" PUSH_SETTINGS_FILE))
        PushGpuLoadD3DKMT = TRUE;

    // Get the number of processors in the system
    NtQuerySystemInformation(SystemBasicInformation, &HwInfoSystemBasicInformation, sizeof(SYSTEM_BASIC_INFORMATION), 0);

    PushSharedMemory->HarwareInformation.Processor.NumberOfCores = HwInfoSystemBasicInformation.NumberOfProcessors;
    
    physicalPages = HwInfoSystemBasicInformation.NumberOfPhysicalPages;
    pageSize = HwInfoSystemBasicInformation.PageSize;

    //byte => megabytes
    PushSharedMemory->HarwareInformation.Memory.Total = (physicalPages * pageSize) / 1048576;
    
    coreListEntry = &PushSharedMemory->HarwareInformation.Processor.coreList;

    for (i = 0; i < PushSharedMemory->HarwareInformation.Processor.NumberOfCores; i++)
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
    
    // Start disk monitoring;
    DiskStartMonitoring();

    CPU_Intialize();
}


VOID RefreshHardwareInfo()
{
    GPU_INFO gpuInfo;

    PhpUpdateCpuInformation();

    GPU_GetInfo(&gpuInfo);

    PushSharedMemory->HarwareInformation.Processor.Speed                = CPU_GetSpeed();
    PushSharedMemory->HarwareInformation.Processor.Load                 = GetCpuLoad();
    PushSharedMemory->HarwareInformation.Processor.MaxCoreUsage         = GetMaxCoreLoad();
    PushSharedMemory->HarwareInformation.Processor.Temperature          = GetCpuTemp();
    PushSharedMemory->HarwareInformation.DisplayDevice.Load             = gpuInfo.Load;
    PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock      = gpuInfo.EngineClock;
    PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClock      = gpuInfo.MemoryClock;
    PushSharedMemory->HarwareInformation.DisplayDevice.Voltage          = gpuInfo.Voltage;
    PushSharedMemory->HarwareInformation.DisplayDevice.Temperature      = gpuInfo.Temperature;
    PushSharedMemory->HarwareInformation.DisplayDevice.FrameBuffer.Used = gpuInfo.MemoryUsed;
    PushSharedMemory->HarwareInformation.DisplayDevice.FrameBuffer.Load = gpuInfo.MemoryUsage;
    PushSharedMemory->HarwareInformation.DisplayDevice.FanSpeed         = gpuInfo.FanSpeed;
    PushSharedMemory->HarwareInformation.Memory.Used                    = GetRamUsed();
    PushSharedMemory->HarwareInformation.Memory.Load                    = GetRamUsage();
    PushSharedMemory->HarwareInformation.Disk.ReadWriteRate             = GetDiskReadWriteRate();
    //hardware.Disk.ResponseTime              = GetDiskResponseTime();

    // We actually get some information from other sources
    //hardware.Processor.MaxThreadUsage = PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage;
}
