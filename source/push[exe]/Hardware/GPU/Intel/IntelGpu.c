#include <push.h>
#include "IntelGpu.h"
#include "..\d3dkmt.h"
//#include "..\CPU\intel.h"


UINT8 IntelGpu_GetLoad();
UINT16 IntelGpu_GetEngineClock();
UINT16 IntelGpu_GetMemoryClock();
UINT16 IntelGpu_GetMaxEngineClock();
UINT16 IntelGpu_GetMaxMemoryClock();
UINT64 IntelGpu_GetTotalMemory();
UINT64 IntelGpu_GetFreeMemory();
UINT8  IntelGpu_GetTemperature();
UINT16 IntelGpu_GetVoltage();
UINT16 IntelGpu_GetFanSpeed();
VOID IntelGpu_ForceMaximumClocks();


#define GT_PERF_STATUS 0x5948
#define GT_FREQUENCY_MULTIPLIER 50


VOID IntelGpu_Initialize()
{

}


UINT16 IntelGpu_GetEngineClock()
{
    UINT32 frequency = 0;

    frequency = ReadGpuRegister(GT_PERF_STATUS);
    frequency = (frequency & 0xff00) >> 8;
    frequency *= GT_FREQUENCY_MULTIPLIER;

    return frequency;
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
    return 0;
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


UINT16 IntelGpu_GetVoltage()
{
    return 0;
}


UINT16 IntelGpu_GetFanSpeed()
{
    return 0;
}


VOID IntelGpu_ForceMaximumClocks()
{

}