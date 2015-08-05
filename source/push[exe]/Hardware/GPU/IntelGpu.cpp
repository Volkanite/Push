#include <sltypes.h>
#include "IntelGpu.h"
#include "d3dkmt.h"
#include "..\CPU\intel.h"


UINT8 IntelGpu_GetLoad();
UINT16 IntelGpu_GetEngineClock();
UINT16 IntelGpu_GetMemoryClock();
UINT16 IntelGpu_GetMaxEngineClock();
UINT16 IntelGpu_GetMaxMemoryClock();
UINT64 IntelGpu_GetTotalMemory();
UINT64 IntelGpu_GetFreeMemory();
UINT8  IntelGpu_GetTemperature();
VOID IntelGpu_ForceMaximumClocks();


VOID IntelGpu_CreateAdapter(GPU_ADAPTER* GpuAdapter)
{
    GpuAdapter->GetEngineClock          = IntelGpu_GetEngineClock;
    GpuAdapter->GetMemoryClock          = IntelGpu_GetMemoryClock;
    GpuAdapter->GetMaximumEngineClock   = IntelGpu_GetMaxEngineClock;
    GpuAdapter->GetMaximumMemoryClock   = IntelGpu_GetMaxMemoryClock;
    GpuAdapter->GetTotalMemory          = IntelGpu_GetTotalMemory;
    GpuAdapter->GetFreeMemory           = IntelGpu_GetFreeMemory;
    GpuAdapter->GetTemperature          = IntelGpu_GetTemperature;
    GpuAdapter->ForceMaximumClocks      = IntelGpu_ForceMaximumClocks;
    GpuAdapter->GetLoad                 = IntelGpu_GetLoad;
}


UINT16 IntelGpu_GetEngineClock()
{
    return 0;
}


UINT16 IntelGpu_GetMemoryClock()
{
    return 0;
}


UINT64 IntelGpu_GetTotalMemory()
{
    return 0;
}


UINT64 IntelGpu_GetFreeMemory()
{
    return 0;
}


UINT8 IntelGpu_GetTemperature()
{
    return GetIntelTemp();
}


UINT8 IntelGpu_GetLoad()
{
    return D3DKMT_GetGpuUsage();
}


UINT16 IntelGpu_GetMaxEngineClock()
{
    return 0;
}


UINT16 IntelGpu_GetMaxMemoryClock()
{
    return 0;
}


VOID IntelGpu_ForceMaximumClocks()
{

}