#include <sl.h>
#include <stdio.h>
#include <push.h>
#include <hwinfo.h>
#include "adl.h"
#include "AmdGpu.h"


#define R6XX_CONFIG_MEMSIZE 0x5428


SlAdl RdnAdl;


extern "C"
UINT16
GetRadeonMemorySize()
{
    return ReadGpuRegister(
        R6XX_CONFIG_MEMSIZE
        );
}


extern "C"
VOID
RdnSetMaxClocks()
{
    RdnAdl.SetMaxClocks();
}


UINT16 AmdGpu_GetEngineClock()
{
    return RdnAdl.GetEngineClock();
}


UINT16 AmdGpu_GetMemoryClock()
{
    return RdnAdl.GetMemoryClock();
}


UINT64 AmdGpu_GetTotalMemory()
{
    return 0;
}


UINT64 AmdGpu_GetFreeMemory()
{
    return 0;
}


UINT8 AmdGpu_GetTemperature()
{
    UINT32 temp;

    temp = ReadGpuRegister(0x730);

    return temp;
}


UINT8 
AmdGpu::GetLoad()
{
    DWORD usage, reg6do;
    FLOAT f1;

    usage = ReadGpuRegister(0x668);

    reg6do = ReadGpuRegister(0x6D0);

    reg6do = (reg6do & 0x0000ffff);

    usage = (usage & 0x0000ffff);

    f1 = usage;

    f1 = f1 * 200.0f;

    f1 = f1 / (float) reg6do;

    return f1;
}


UINT16 
AmdGpu::GetMaximumEngineClock()
{
    return RdnAdl.GetEngineClockMax();
}


UINT16 
AmdGpu::GetMaximumMemoryClock()
{
    return RdnAdl.GetMemoryClockMax();
}


VOID 
AmdGpu::ForceMaximumClocks()
{

}