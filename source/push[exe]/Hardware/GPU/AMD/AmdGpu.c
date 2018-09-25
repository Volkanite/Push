#include <push.h>
#include "adl.h"
#include "AmdGpu.h"
#include "..\d3dkmt.h"


#define R6XX_CONFIG_MEMSIZE     0x5428
#define AMD_TEMP                0x730
#define CG_MULT_THERMAL_STATUS  0x740
#define CG_THERMAL_STATUS       0x7F4
#define AMD_USAGE1              0x668
#define AMD_USAGE2              0x6D0
#define ASIC_T_MASK             0x1FF

extern WORD GPU_DeviceId;
BOOLEAN ADL_Initialized;

UINT8 AmdGpu_GetLoad();
UINT16 AmdGpu_GetEngineClock();
UINT16 AmdGpu_GetMemoryClock();
UINT16 AmdGpu_GetMaxEngineClock();
UINT16 AmdGpu_GetMaxMemoryClock();
UINT64 AmdGpu_GetTotalMemory();
UINT64 AmdGpu_GetFreeMemory();
UINT8  AmdGpu_GetTemperature();
VOID AmdGpu_ForceMaximumClocks();

VOID AtomBios_Initialize();
UINT32 AtomBios_GetEngineClock();
UINT32 AtomBios_GetMemoryClock();
UINT32 radeon_atom_get_engine_clock();


VOID AmdGpu_Initialize()
{
    ADL_Initialized = Adl_Initialize();

    //Prefer ADL but if not then AtomBios
    if (!ADL_Initialized && PushDriverLoaded)
    {
        AtomBios_Initialize();
    }

    //this is a keeper until further notice
    D3DKMTInitialize();
}


VOID AmdGpu_GetInfo( GPU_INFO* Information )
{
    if (ADL_Initialized)
        Adl_GetInfo(Information);

    Information->Temperature = AmdGpu_GetTemperature();
    

    if (!ADL_Initialized)
    {
        Information->EngineClock = AtomBios_GetEngineClock();
        Information->MemoryClock = AtomBios_GetMemoryClock();
    }
}


UINT16 AmdGpu_GetEngineClockMax()
{
    UINT16 clock;

    clock = AtomBios_GetEngineClock();

    return clock;
}


UINT16 AmdGpu_GetMemoryClockMax()
{
    UINT16 clock;

    clock = AtomBios_GetMemoryClock();

    return clock;
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
    UINT64 used, total;

    used = D3DKMTGetMemoryUsage();

    if (PushDriverLoaded)
    {
        total = AmdGpu_GetTotalMemory();
    }
    else
    {
        total = D3DKMT_GetTotalMemory();
    }

    return total - used;
}


UINT8 AmdGpu_GetTemperature()
{
    UINT32 temp;

    switch (GPU_DeviceId)
    {
    case 0x95C5:
        temp = ReadGpuRegister(CG_THERMAL_STATUS) & ASIC_T_MASK;
        break;
    default:
        temp = ReadGpuRegister(AMD_TEMP);
        break;
    }
    
    return temp;
}


UINT8 AmdGpu_GetLoad()
{
    if (GPU_DeviceId == 0x9832)
    {
        GPU_INFO activity;

        Adl_GetInfo(&activity);

        return activity.Load;
    }
    else
    {
        DWORD usage, reg6do;
        FLOAT f1;

        usage = ReadGpuRegister(AMD_USAGE1);

        reg6do = ReadGpuRegister(AMD_USAGE2);

        reg6do = (reg6do & 0x0000ffff);

        usage = (usage & 0x0000ffff);

        f1 = usage;

        f1 = f1 * 200.0f;

        f1 = f1 / (float)reg6do;

        return f1;
    }
}


VOID AmdGpu_ForceMaximumClocks()
{
    Adl_SetMaxClocks();
}


