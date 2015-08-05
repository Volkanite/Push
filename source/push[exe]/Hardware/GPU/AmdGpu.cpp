#include <sl.h>
#include <stdio.h>
#include <push.h>
#include <hardware.h>
#include "adl.h"
#include "AmdGpu.h"
#include "d3dkmt.h"


#define R6XX_CONFIG_MEMSIZE 0x5428


UINT8 AmdGpu_GetLoad();
UINT16 AmdGpu_GetEngineClock();
UINT16 AmdGpu_GetMemoryClock();
UINT16 AmdGpu_GetMaxEngineClock();
UINT16 AmdGpu_GetMaxMemoryClock();
UINT64 AmdGpu_GetTotalMemory();
UINT64 AmdGpu_GetFreeMemory();
UINT8  AmdGpu_GetTemperature();
VOID AmdGpu_ForceMaximumClocks();


VOID AmdGpu_CreateAdapter( GPU_ADAPTER* GpuAdapter )
{
    Adl_Initialize();
    D3DKMTInitialize();

    GpuAdapter->GetEngineClock          = AmdGpu_GetEngineClock;
    GpuAdapter->GetMemoryClock          = AmdGpu_GetMemoryClock;
    GpuAdapter->GetMaximumEngineClock   = AmdGpu_GetMaxEngineClock;
    GpuAdapter->GetMaximumMemoryClock   = AmdGpu_GetMaxMemoryClock;
    GpuAdapter->GetTotalMemory          = AmdGpu_GetTotalMemory;
    GpuAdapter->GetFreeMemory           = AmdGpu_GetFreeMemory;
    GpuAdapter->GetTemperature          = AmdGpu_GetTemperature;
    GpuAdapter->ForceMaximumClocks      = AmdGpu_ForceMaximumClocks;
    GpuAdapter->GetLoad                 = AmdGpu_GetLoad;
}


VOID RdnSetMaxClocks()
{
    Adl_SetMaxClocks();
}


UINT16 AmdGpu_GetEngineClock()
{
    return Adl_GetEngineClock();
}


UINT16 AmdGpu_GetMemoryClock()
{
    return Adl_GetMemoryClock();
}


UINT64 AmdGpu_GetTotalMemory()
{
    UINT16 megabytes;
    
    megabytes = ReadGpuRegister(R6XX_CONFIG_MEMSIZE);

    return megabytes * 1048576; //megabytes -> bytes
}


UINT64 AmdGpu_GetFreeMemory()
{
    UINT64 used = D3DKMTGetMemoryUsage();
    UINT64 total = AmdGpu_GetTotalMemory();

    return total - used;
}


UINT8 AmdGpu_GetTemperature()
{
    UINT32 temp;

    temp = ReadGpuRegister(0x730);

    return temp;
}


UINT8 AmdGpu_GetLoad()
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


UINT16 AmdGpu_GetMaxEngineClock()
{
    return Adl_GetEngineClockMax();
}


UINT16 AmdGpu_GetMaxMemoryClock()
{
    return Adl_GetMemoryClockMax();
}


VOID AmdGpu_ForceMaximumClocks()
{
    Adl_SetMaxClocks();
}