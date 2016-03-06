#include <sl.h>
#include <ring0.h>

#include "cpu.h"


#define IA32_THERM_STATUS_MSR 0x019C


UINT8 Intel_GetTemperature()
{
    INT32 retTemp = 0;
    INT32 i= 0;


    for (i = 0; i < 4; i++)
    {
        DWORD eax;
        DWORD mask          = 0;
        float deltaT;   
        float tjMax;        
        float tSlope;   
        float coreTemp; 
        void *hThread       = NULL;

        hThread = GetCurrentThread();
        
        mask = SetThreadAffinityMask(hThread, 1UL << i);

        eax = CPU_ReadMsr(IA32_THERM_STATUS_MSR);

        //memcpy(&eax, outBuf, 4);
        
        SetThreadAffinityMask(hThread, mask);

        deltaT  = ((eax & 0x007F0000) >> 16);
        tjMax       = 100.0;
        tSlope  = 1.0;
        coreTemp    = tjMax - tSlope * deltaT;

        if((INT32) coreTemp > retTemp)
        {
            retTemp = coreTemp;
        }
    }

    return retTemp;
}