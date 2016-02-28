#include <sl.h>
#include <ring0.h>


#define PCI_BUS 0
#define PCI_BASE_DEVICE 0x18
#define MISCELLANEOUS_CONTROL_FUNCTION 3
#define REPORTED_TEMPERATURE_CONTROL_REGISTER 0xA4

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