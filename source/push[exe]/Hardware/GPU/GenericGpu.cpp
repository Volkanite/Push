#include <sltypes.h>
#include "GenericGpu.h"
#include "d3dkmt.h"


UINT8 GenericGpu_GetLoad();
UINT16 GenericGpu_GetEngineClock();
UINT16 GenericGpu_GetMemoryClock();
UINT16 GenericGpu_GetMaxEngineClock();
UINT16 GenericGpu_GetMaxMemoryClock();
UINT64 GenericGpu_GetTotalMemory();
UINT64 GenericGpu_GetFreeMemory();
UINT8  GenericGpu_GetTemperature();
VOID GenericGpu_ForceMaximumClocks();


VOID GenericGpu_CreateAdapter(GPU_ADAPTER* GpuAdapter)
{
    GpuAdapter->GetEngineClock          = GenericGpu_GetEngineClock;
    GpuAdapter->GetMemoryClock          = GenericGpu_GetMemoryClock;
    GpuAdapter->GetMaximumEngineClock   = GenericGpu_GetMaxEngineClock;
    GpuAdapter->GetMaximumMemoryClock   = GenericGpu_GetMaxMemoryClock;
    GpuAdapter->GetTotalMemory          = GenericGpu_GetTotalMemory;
    GpuAdapter->GetFreeMemory           = GenericGpu_GetFreeMemory;
    GpuAdapter->GetTemperature          = GenericGpu_GetTemperature;
    GpuAdapter->ForceMaximumClocks      = GenericGpu_ForceMaximumClocks;
    GpuAdapter->GetLoad                 = GenericGpu_GetLoad;
}


UINT16 GenericGpu_GetEngineClock()
{
    return 0;
}


UINT16 GenericGpu_GetMemoryClock()
{
    return 0;
}


UINT64 GenericGpu_GetTotalMemory()
{
    return 0;
}


UINT64 GenericGpu_GetFreeMemory()
{
    return 0;
}


UINT8 GenericGpu_GetTemperature()
{
    return 0;
}


UINT8 GenericGpu_GetLoad()
{
    return D3DKMT_GetGpuUsage();
}


UINT16 GenericGpu_GetMaxEngineClock()
{
    return 0;
}


UINT16 GenericGpu_GetMaxMemoryClock()
{
    return 0;
}


VOID GenericGpu_ForceMaximumClocks()
{

}