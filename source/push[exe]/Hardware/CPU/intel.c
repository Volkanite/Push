#include <sl.h>
#include <ring0.h>

#include "cpu.h"


#define IA32_PERF_STATUS        0x0198
#define IA32_THERM_STATUS       0x019C
#define IA32_TEMPERATURE_TARGET 0x01A2
#define TURBO_RATIO_LIMIT       0x01AD


float TjMax;


VOID IntelCPU_Initialize()
{
    DWORD eax;

    eax = CPU_ReadMsr(IA32_TEMPERATURE_TARGET);

    TjMax = (eax >> 16) & 0xFF;
}


UINT8 Intel_GetTemperature()
{
    INT32 retTemp = 0;
    INT32 i= 0;


    for (i = 0; i < PushSharedMemory->HarwareInformation.Processor.NumberOfCores; i++)
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
* IA32_PERF_STATUS MSR (0x198) Bits 31:0 indicates the
* processor speed.
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
* TURBO_RATIO_LIMIT MSR (0x1AD) Bits 31:0 indicates the
* factory configured values for of 1-core, 2-core, 3-core
* and 4-core turbo ratio limits for all processors.
*
* 7:0 - Maximum turbo ratio limit of 1 core active.
*
*/

UINT16 Intel_GetMaxSpeed()
{
    DWORD eax;
    unsigned __int32 multiplier;
    unsigned __int16 coreClock;

    eax = CPU_ReadMsr(TURBO_RATIO_LIMIT);

    multiplier = (eax & 0xff);
    coreClock = (float)(multiplier * 100.0f);

    return coreClock;
}