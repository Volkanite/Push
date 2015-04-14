#include <sl.h>
#include "d3dkmt.h"


typedef struct NVAPI_PRIVATE_MEMORY_DATA
{
    DWORD Dummy1;
    DWORD Dummy2;
    DWORD Dummy3;
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

    memoryData.Dummy1 = 0x4E564441;
    memoryData.Dummy2 = 0x10002;
    memoryData.Dummy3 = 0x4C;
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
    
    memoryData.Dummy1 = 0x4E564441;
    memoryData.Dummy2 = 0x10002;
    memoryData.Dummy3 = 0x4C;
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