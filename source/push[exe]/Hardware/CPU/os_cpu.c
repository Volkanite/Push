#include <push.h>


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
    ((DltMgr)->Delta = (NewValue)-(DltMgr)->Value, \
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
extern SYSTEM_BASIC_INFORMATION    HwInfoSystemBasicInformation;


FLOAT
GetCpuUsage(UINT8 Cpu);


UINT8
GetCpuLoad()
{
    return (PhCpuUserUsage + PhCpuKernelUsage) * 100;
}


FLOAT
GetCpuUsage(UINT8 Cpu)
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


VOID PhProcessProviderInitialization()
{
    FLOAT *usageBuffer;
    PPH_UINT64_DELTA deltaBuffer;

    PhCpuInformation = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*)RtlAllocateHeap(
        PushHeapHandle,
        0,
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)* (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors
        );

    usageBuffer = (FLOAT*)RtlAllocateHeap(
        PushHeapHandle,
        0,
        sizeof(FLOAT)* (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors * 2
        );

    deltaBuffer = (PPH_UINT64_DELTA)RtlAllocateHeap(
        PushHeapHandle,
        0,
        sizeof(PH_UINT64_DELTA)* (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors * 3
        );

    PhCpusKernelUsage = usageBuffer;
    PhCpusUserUsage = PhCpusKernelUsage + (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors;

    PhCpusKernelDelta = deltaBuffer;
    PhCpusUserDelta = PhCpusKernelDelta + (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors;
    PhCpusIdleDelta = PhCpusUserDelta + (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors;

    memset(deltaBuffer, 0, sizeof(PH_UINT64_DELTA)* (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors);
}


VOID PhpUpdateCpuInformation()
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
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)* (ULONG)HwInfoSystemBasicInformation.NumberOfProcessors,
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