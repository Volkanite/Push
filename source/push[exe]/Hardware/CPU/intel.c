#include <sl.h>
#include <ring0.h>

#include "cpu.h"
#include "intel.h"


#define MSR_CORE_THREAD_COUNT   0x0035
#define IA32_PERF_STATUS        0x0198
#define IA32_THERM_STATUS       0x019C
#define MSR_TEMPERATURE_TARGET  0x01A2
#define MSR_TURBO_RATIO_LIMIT   0x01AD
#define FSB_CLOCK_VCC           0x00CE


int CoreCount;
float TjMax;

int GetCoreCount();
float GetTjMax();
extern WORD Model;


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
    DWORD eax, edx;
    unsigned __int8 cores;

    CPU_ReadMsr(MSR_CORE_THREAD_COUNT, &eax, &edx);

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
    DWORD eax, edx;
    float tjMax;

    CPU_ReadMsr(MSR_TEMPERATURE_TARGET, &eax, &edx);

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
        DWORD eax, edx;
        DWORD mask          = 0;
        float deltaT;   
               
        float tSlope;   
        float coreTemp; 
        void *hThread       = NULL;

        hThread = NtCurrentThread();
        
        mask = SetThreadAffinityMask(hThread, 1UL << i);

        CPU_ReadMsr(IA32_THERM_STATUS, &eax, &edx);
        
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
* 16:31 - Max and Min FIDs
*
* Register EAX bits 0..7: the current VID
* Register EAX bits 8..15: the current FID
* Register EDX bits 0..7: The maximum VID value.
* Register EDX bits 8..15: The maximum FID value (multiplier).
* Register EDX bits 16..23: The minimum VID value.
* Register EDX bits 24..31: The minimum FID value (multiplier).
*
* MSR_TURBO_RATIO_LIMIT MSR (0x1AD) Bits 31:0 [Scope: Package]
* Indicates the factory configured values for of 1-core, 2-core, 3-core and 4-core
* turbo ratio limits for all processors.
*
* 7:0 - Maximum turbo ratio limit of 1 core active.
*
*/

UINT16 Intel_GetSpeed( INTEL_SPEED_INDEX Index )
{
    DWORD eax, edx;
    float multiplier;
    float busSpeed;
    unsigned __int16 coreClock;

    busSpeed = 100.0f;

    switch (Index)
    {
    case INTEL_SPEED_LFM:
        CPU_ReadMsr(FSB_CLOCK_VCC, &eax, &edx);
        multiplier = (edx >> 8) & 0xff;
        break;
    case INTEL_SPEED_HFM:
        switch (Model)
        {
        case 0x17: // Intel Core 2 (45nm) - Core
            CPU_ReadMsr(IA32_PERF_STATUS, &eax, &edx);
            multiplier = ((edx >> 8) & 0x1f) + 0.5 * ((edx >> 14) & 1);
            busSpeed = 266.0f;
            break;
        case 0x2A: // Intel Core i5, i7 2xxx LGA1155 (32nm) - SandyBridge
        default:
            CPU_ReadMsr(FSB_CLOCK_VCC, &eax, &edx);
            multiplier = (eax >> 8) & 0xff;
            break;
        }
        break;
    case INTEL_SPEED_TURBO:
        CPU_ReadMsr(MSR_TURBO_RATIO_LIMIT, &eax, &edx);
        multiplier = (eax & 0xff);
        break;
    case INTEL_SPEED_STATUS:
        CPU_ReadMsr(IA32_PERF_STATUS, &eax, &edx);
        multiplier = (eax >> 8) & 0xff;
        break;
    default:
        break;
    }

    coreClock = (float)(multiplier * busSpeed);

    return coreClock;
}