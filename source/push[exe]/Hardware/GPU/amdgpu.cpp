#include <sltypes.h>
#include <slntuapi.h>
#include <slc.h>
#include <stdio.h>
#include <pushbase.h>
#include <hwinfo.h>
#include <sladl.h>
#include "AmdGpu.h"


#define R6XX_CONFIG_MEMSIZE 0x5428


SlAdl RdnAdl;


/*extern "C"
UINT8
GetRadeonTemp()
{
    UINT32 temp;

    temp = ReadGpuRegister(0x730);

    //printf("Tremp %x\n", temp);

    return temp;
}*/


extern "C"
UINT16
GetRadeonMemorySize()
{
    return ReadGpuRegister(R6XX_CONFIG_MEMSIZE);
}


extern "C"
UINT16
RdnGetEngineClock()
{
    return RdnAdl.GetEngineClock();
}


extern "C"
UINT16
RdnGetMemoryClock()
{
    return RdnAdl.GetMemoryClock();
}


extern "C"
UINT16
RdnGetEngineClockMax()
{
    return RdnAdl.GetEngineClockMax();
}


extern "C"
UINT16
RdnGetMemoryClockMax()
{
    return RdnAdl.GetMemoryClockMax();
}


extern "C"
VOID
RdnSetMaxClocks()
{
    RdnAdl.SetMaxClocks();
}


UINT16 
AmdGpu::GetEngineClock()
{
    return 0;
}


UINT16 
AmdGpu::GetMemoryClock()
{
    return 0;
}


UINT64 
AmdGpu::GetTotalMemory()
{
    return 0;
}


UINT64 
AmdGpu::GetFreeMemory()
{
    return 0;
}


UINT8 
AmdGpu::GetTemperature()
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
    return 0;
}


UINT16 
AmdGpu::GetMaximumMemoryClock()
{
    return 0;
}


VOID 
AmdGpu::ForceMaximumClocks()
{

}