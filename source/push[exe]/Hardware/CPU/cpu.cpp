#include <sl.h>
#include <vendors.h>
#include <intrin.h>
#include <string.h>

#include "amd.h"
#include "intel.h"


WORD Vendor;


BOOLEAN Cpuid(
    DWORD index, 
    DWORD *pEAX, 
    DWORD *pEBX,
    DWORD *pECX, 
    DWORD *pEDX
    )
{
    int info[4];
    __asm{mov ecx, 0}
    __cpuid(info, index);
    *pEAX = info[0];
    *pEBX = info[1];
    *pECX = info[2];
    *pEDX = info[3];

    return TRUE;
}


BOOLEAN CpuidTx(
    DWORD index, 
    DWORD* eax, 
    DWORD* ebx, 
    DWORD* ecx, 
    DWORD* edx, 
    DWORD threadAffinityMask
    )
{
    BOOLEAN result = FALSE;
    DWORD   mask = 0;
    HANDLE  hThread = NULL;

    hThread = GetCurrentThread();
    
    mask = SetThreadAffinityMask(
        hThread, 
        threadAffinityMask
        );
    
    if (mask == 0)
    {
        return FALSE;
    }

    result = Cpuid(
        index, 
        eax, 
        ebx, 
        ecx, 
        edx
        );


    SetThreadAffinityMask(
        hThread, 
        mask
        );

    return result;
}


VOID CPU_Intialize()
{
    DWORD  eax, ebx, ecx, edx;
    CHAR name[12];

    CpuidTx(
        0, 
        &eax, 
        &ebx, 
        &ecx, 
        &edx,
        1UL
        );

    Memory::Copy(name, &ebx, 4);
    Memory::Copy(name + 4, &edx, 4);
    Memory::Copy(name + 8, &ecx, 4);

    if (strncmp(name, "GenuineIntel", 12) == 0)
    {
        Vendor = INTEL;
    }
    else if (strncmp(name, "AuthenticAMD", 12) == 0)
    {
        Vendor = AMD;

        AMD_Initialize();
    }
}


UINT8 CPU_GetTemperature()
{
    switch (Vendor)
    {
    case INTEL:
        return Intel_GetTemperature();
        break;
    case AMD:
        return AMD_GetTemperature();
        break;
    default:
        return 0;
        break;
    }
}