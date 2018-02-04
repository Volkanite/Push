#include <sl.h>
#include <ring0.h>

#include "cpu.h"


#define MSR_CORE_THREAD_COUNT   0x0035
#define IA32_PERF_STATUS        0x0198
#define IA32_THERM_STATUS       0x019C
#define MSR_TEMPERATURE_TARGET  0x01A2
#define MSR_TURBO_RATIO_LIMIT   0x01AD


int CoreCount;
float TjMax;

int GetCoreCount();
float GetTjMax();


VOID IntelCPU_Initialize()
{
    CoreCount = GetCoreCount();
    TjMax = GetTjMax();
}


/*
* MSR_CORE_THREAD_COUNT MSR (0x35) Bits 31:0 [Scope: Package]
* Configured State of Enabled Processor Core Count and Logical Processor Count
*
* 31:16 [CORE_COUNT] - The number of processor cores that are currently enabled (by
* either factory configuration or BIOS configuration) in the physical
* package.
*
*/

int GetCoreCount()
{
    DWORD eax;
    unsigned __int8 cores;

    eax = CPU_ReadMsr(MSR_CORE_THREAD_COUNT);

    cores = (eax >> 16) & 0xFFFF;

    return cores;
}


/*
* MSR_TEMPERATURE_TARGET MSR (0x1A2) Bits 31:0 [Scope: Thread]
* Temperature Target
*
* 23:16 [Temperature Target] - The minimum temperature at which PROCHOT# will be asserted.
* The value is degree C.
*
*/

float GetTjMax()
{
    DWORD eax;
    float tjMax;

    eax = CPU_ReadMsr(MSR_TEMPERATURE_TARGET);

    tjMax = (eax >> 16) & 0xFF;

    return tjMax;
}


/*
* IA32_THERM_STATUS MSR (0x19C) Bits 31:0 [Scope: Core]
* Contains status information about the processor's thermal sensor and automatic
* thermal monitoring facilities.
*
* 15:0 - Current Temperature Status Value
*
*/

UINT8 Intel_GetTemperature()
{
    INT32 retTemp = 0;
    INT32 i= 0;


    for (i = 0; i < CoreCount; i++)
    {
        DWORD eax;
        DWORD mask          = 0;
        float deltaT;   
               
        float tSlope;   
        float coreTemp; 
        void *hThread       = NULL;

        hThread = NtCurrentThread();
        
        mask = SetThreadAffinityMask(hThread, 1UL << i);

        eax = CPU_ReadMsr(IA32_THERM_STATUS);
        
        SetThreadAffinityMask(hThread, mask);

        deltaT  = ((eax & 0x007F0000) >> 16);       
        tSlope  = 1.0;
        coreTemp    = TjMax - tSlope * deltaT;

        if((INT32) coreTemp > retTemp)
        {
            retTemp = coreTemp;
        }
    }

    return retTemp;
}


/*
* IA32_PERF_STATUS MSR (0x198) Bits 31:0 [Scope: Core]
* Indicates the processor speed.
*
* 15:0 - Current Performance State Value
*
*/

UINT16 Intel_GetSpeed()
{
    DWORD eax;
    unsigned __int32 multiplier;
    unsigned __int16 coreClock;

    eax = CPU_ReadMsr(IA32_PERF_STATUS);

    multiplier = (eax >> 8) & 0xff;
    coreClock = (float)(multiplier * 100.0f);

    return coreClock;
}


/*
* MSR_TURBO_RATIO_LIMIT MSR (0x1AD) Bits 31:0 [Scope: Package]
* Indicates the factory configured values for of 1-core, 2-core, 3-core and 4-core 
* turbo ratio limits for all processors.
*
* 7:0 - Maximum turbo ratio limit of 1 core active.
*
*/

UINT16 Intel_GetMaxSpeed()
{
    DWORD eax;
    unsigned __int32 multiplier;
    unsigned __int16 coreClock;

    eax = CPU_ReadMsr(MSR_TURBO_RATIO_LIMIT);

    multiplier = (eax & 0xff);
    coreClock = (float)(multiplier * 100.0f);

    return coreClock;
}