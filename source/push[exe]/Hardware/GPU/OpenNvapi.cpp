#include <sl.h>
#include "d3dkmt.h"


#define NVAPI_PRIVATE_MEMORY_DATA_IOCTL 0x1000012
#define NVAPI_PRIVATE_USAGE_DATA_IOCTL  0x7000060


typedef struct _NVAPI_PRIVATE_DATA_HEADER
{
    DWORD Dummy1;
    DWORD Dummy2;
    DWORD Size;
    DWORD Dummy3;
    DWORD ControlCode;

}NVAPI_PRIVATE_DATA_HEADER;

typedef struct _NVAPI_PRIVATE_USAGE_DATA
{
    NVAPI_PRIVATE_DATA_HEADER Header;
    BYTE Dummy[164];

}NVAPI_PRIVATE_USAGE_DATA; //184 bytes

typedef struct _NVAPI_PRIVATE_MEMORY_DATA
{
    NVAPI_PRIVATE_DATA_HEADER Header;
    BYTE Dummy6[28];
    DWORD Total;    //kilobytes
    BYTE Dummy7[4];
    DWORD Free;     //kilobytes
    BYTE Dummy8[16];

}NVAPI_PRIVATE_MEMORY_DATA; //76 bytes


UINT16 OpenNvapi_GetEngineClock()
{
    return 0;
}


UINT16 OpenNvapi_GetMemoryClock()
{
    return 0;
}


VOID OpenNvapi_GetClocks()
{

}


UINT16 OpenNvapi_GetMaxEngineClock()
{
    return 0;
}


UINT16 OpenNvapi_GetMaxMemoryClock()
{
    return 0;
}


VOID OpenNvapi_GetMaximumClocks()
{

}


VOID OpenNvapi_ForceMaximumClocks()
{

}


UINT64 OpenNvapi_GetTotalMemory()
{
    NVAPI_PRIVATE_MEMORY_DATA memoryData = { 0 };

    memoryData.Header.Dummy1 = 0x4E564441;
    memoryData.Header.Dummy2 = 0x10002;
    memoryData.Header.Size = sizeof(NVAPI_PRIVATE_MEMORY_DATA);
    memoryData.Header.Dummy3 = 0x4E562A2A;
    memoryData.Header.ControlCode = NVAPI_PRIVATE_MEMORY_DATA_IOCTL;
    memoryData.Dummy8[15] = 0xB4;

    D3DKMT_GetPrivateDriverData(
        &memoryData,
        sizeof(NVAPI_PRIVATE_MEMORY_DATA)
        );

    return memoryData.Total * 1024; //kilobytes -> bytes
}


UINT64 OpenNvapi_GetFreeMemory()
{
    NVAPI_PRIVATE_MEMORY_DATA memoryData = { 0 };
    
    memoryData.Header.Dummy1 = 0x4E564441;
    memoryData.Header.Dummy2 = 0x10002;
    memoryData.Header.Size = sizeof(NVAPI_PRIVATE_MEMORY_DATA);
    memoryData.Header.Dummy3 = 0x4E562A2A;
    memoryData.Header.ControlCode = NVAPI_PRIVATE_MEMORY_DATA_IOCTL;
    memoryData.Dummy8[15] = 0xB4;

    D3DKMT_GetPrivateDriverData(
        &memoryData,
        sizeof(NVAPI_PRIVATE_MEMORY_DATA)
        );

    return memoryData.Free * 1024; //kilobytes -> bytes
}


VOID OpenNvapi_GetMemoryInformation()
{

}


UINT8 OpenNvapi_GetTemperature()
{
    return 0;
}


UINT8 OpenNvapi_GetLoad()
{
    NVAPI_PRIVATE_USAGE_DATA usageData = { 0 };
    
    usageData.Header.Dummy1 = 0x4E564441;
    usageData.Header.Dummy2 = 0x10002;
    usageData.Header.Size = sizeof(NVAPI_PRIVATE_USAGE_DATA);
    usageData.Header.Dummy3 = 0x4E562A2A; 
    usageData.Header.ControlCode = NVAPI_PRIVATE_USAGE_DATA_IOCTL;

    D3DKMT_GetPrivateDriverData(
        &usageData,
        sizeof(NVAPI_PRIVATE_USAGE_DATA)
        );

    return 0;
}