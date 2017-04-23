#include <sl.h>
#include <stdio.h>
#include <push.h>
#include <hardware.h>
#include "adl.h"
#include "AmdGpu.h"
#include "d3dkmt.h"


#define R6XX_CONFIG_MEMSIZE 0x5428
WORD DeviceId;

UINT8 AmdGpu_GetLoad();
UINT16 AmdGpu_GetEngineClock();
UINT16 AmdGpu_GetMemoryClock();
UINT16 AmdGpu_GetMaxEngineClock();
UINT16 AmdGpu_GetMaxMemoryClock();
UINT64 AmdGpu_GetTotalMemory();
UINT64 AmdGpu_GetFreeMemory();
UINT8  AmdGpu_GetTemperature();
VOID AmdGpu_ForceMaximumClocks();


VOID AmdGpu_Initialize()
{
    Adl_Initialize();
    D3DKMTInitialize();
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


UINT16 AmdGpu_GetEngineClockMax()
{
    return Adl_GetEngineClockMax();
}


UINT16 AmdGpu_GetMemoryClockMax()
{
    return Adl_GetMemoryClockMax();
}


UINT16 AmdGpu_GetVoltageMax()
{
    return Adl_GetVoltageMax();
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
    if (DeviceId == 0x9832)
    {
        return Adl_GetActivity();
    }
    else
    {
        DWORD usage, reg6do;
        FLOAT f1;

        usage = ReadGpuRegister(0x668);

        reg6do = ReadGpuRegister(0x6D0);

        reg6do = (reg6do & 0x0000ffff);

        usage = (usage & 0x0000ffff);

        f1 = usage;

        f1 = f1 * 200.0f;

        f1 = f1 / (float)reg6do;

        return f1;
    }
}


UINT16 AmdGpu_GetMaxEngineClock()
{
    return Adl_GetEngineClockMax();
}


UINT16 AmdGpu_GetMaxMemoryClock()
{
    return Adl_GetMemoryClockMax();
}


UINT16 AmdGpu_GetVoltage()
{
    return Adl_GetVoltage();
}


VOID AmdGpu_ForceMaximumClocks()
{
    Adl_SetMaxClocks();
}


