#include <sl.h>
#include <ring0.h>

#include "cpu.h"


#define IA32_PERF_STATUS        0x0198
#define IA32_THERM_STATUS       0x019C
#define IA32_TEMPERATURE_TARGET 0x01A2


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