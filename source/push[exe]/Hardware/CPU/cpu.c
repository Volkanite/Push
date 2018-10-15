#include <sl.h>
#include <vendors.h>
//#include <intrin.h>
#include <pushbase.h>
#include <push.h>

#include "amd.h"
#include "intel.h"
#include "os_cpu.h"
#include "..\wr0.h"


WORD Vendor;
WORD Model;
extern VOID *R0DriverHandle;
#define CPUID_VENDORID          0
#define CPUID_PROCESSOR_INFO    1


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

    hThread = NtCurrentThread();
    
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
        CPUID_VENDORID, 
        &eax, 
        &ebx, 
        &ecx, 
        &edx,
        1UL
        );

    Memory_Copy(name, &ebx, 4);
    Memory_Copy(name + 4, &edx, 4);
    Memory_Copy(name + 8, &ecx, 4);

    if (strncmp(name, "GenuineIntel", 12) == 0)
    {
        Vendor = INTEL;

        IntelCPU_Initialize();
    }
    else if (strncmp(name, "AuthenticAMD", 12) == 0)
    {
        Vendor = AMD;

        AMD_Initialize();
    }

    CpuidTx(
        CPUID_PROCESSOR_INFO,
        &eax,
        &ebx,
        &ecx,
        &edx,
        1UL
        );

    Model = ((eax & 0x0F0000) >> 12) +
        ((eax & 0xF0) >> 4);
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


UINT8 CPU_GetTemperatureMaximal()
{
    switch (Vendor)
    {
    case INTEL:
        return Intel_GetTemperatureMaximal();
        break;
    case AMD:
        return 75;
        break;
    default:
        return 0;
        break;
    }
}


UINT16 CPU_GetSpeed()
{
    switch (Vendor)
    {
    case AMD:
        return AMD_GetSpeed();
        break;
    default:
        return Intel_GetSpeed(INTEL_SPEED_STATUS);
        break;
    }
}


UINT16 CPU_GetNormalSpeed()
{
    if (!PushDriverLoaded && !Wr0DriverLoaded)
    {
        return OsGetCpuSpeed();
    }

    switch (Vendor)
    {
    case AMD:
        return AMD_GetSpeed();
        break;
    default:
        return Intel_GetSpeed(INTEL_SPEED_HFM);
        break;
    }
}


UINT16 CPU_GetMaxSpeed()
{
    switch (Vendor)
    {
    case AMD:
        return AMD_GetSpeed();
        break;
    default:
        return Intel_GetSpeed(INTEL_SPEED_TURBO);
        break;
    }
}