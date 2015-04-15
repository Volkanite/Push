#include <sl.h>
#include "d3dkmt.h"


typedef struct _NVAPI_PRIVATE_DATA_HEADER
{
    DWORD Dummy1;
    DWORD Dummy2;
    DWORD Size;

}NVAPI_PRIVATE_DATA_HEADER;

typedef struct _NVAPI_PRIVATE_USAGE_DATA
{
    BYTE Dummy[184];

}NVAPI_PRIVATE_USAGE_DATA; //184 bytes

typedef struct _NVAPI_PRIVATE_MEMORY_DATA
{
    NVAPI_PRIVATE_DATA_HEADER Header;
    DWORD Dummy4;
    DWORD Dummy5;
    BYTE Dummy6[28];
    DWORD Total;    //kilobytes
    BYTE Dummy7[4];
    DWORD Free;     //kilobytes
    BYTE Dummy8[16];

}NVAPI_PRIVATE_MEMORY_DATA; //76 bytes


UINT16 OpenNvapi_GetEngineClock()
{
    
}


UINT16 OpenNvapi_GetMemoryClock()
{
    
}


VOID OpenNvapi_GetClocks()
{

}


UINT16 OpenNvapi_GetMaxEngineClock()
{

}


UINT16 OpenNvapi_GetMaxMemoryClock()
{

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
    memoryData.Dummy4 = 0x4E562A2A;
    memoryData.Dummy5 = 0x1000012;
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
    memoryData.Dummy4 = 0x4E562A2A;
    memoryData.Dummy5 = 0x1000012;
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

}


UINT8 OpenNvapi_GetLoad()
{
    
}