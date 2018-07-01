#include <sl.h>
#include <ring0.h>

#include "cpu.h"


#define PCI_BUS 0
#define PCI_BASE_DEVICE 0x18
#define MISCELLANEOUS_CONTROL_FUNCTION 3
#define REPORTED_TEMPERATURE_CONTROL_REGISTER 0xA4
#define COFVID_STATUS 0xC0010071


DWORD MiscellaneousControlAddress;


DWORD GetPciAddress(BYTE bus, BYTE device, BYTE function) 
{
    return (DWORD)(((bus & 0xFF) << 8) | ((device & 0x1F) << 3) | (function & 7));
}


VOID AMD_Initialize()
{
    MiscellaneousControlAddress = GetPciAddress(
        PCI_BUS, 
        PCI_BASE_DEVICE, 
        MISCELLANEOUS_CONTROL_FUNCTION
        );
}


UINT8 AMD_GetTemperature()
{
    DWORD value;

    R0ReadPciConfig(
        MiscellaneousControlAddress, 
        REPORTED_TEMPERATURE_CONTROL_REGISTER, 
        (BYTE*)&value, 
        sizeof(DWORD)
        );

    return ((value >> 21) & 0x7FF) / 8.0f;
}


UINT16 AMD_GetSpeed()
{
    DWORD eax, edx;
    unsigned __int32 cpuDid;
    unsigned __int32 cpuFid;
    double multiplier;

    CPU_ReadMsr(COFVID_STATUS, &eax, &edx);
    
    cpuDid = (eax >> 6) & 7;
    cpuFid = eax & 0x1F;

    multiplier = (cpuFid + 0x10) >> cpuDid;

    return 99.81f * multiplier;
}