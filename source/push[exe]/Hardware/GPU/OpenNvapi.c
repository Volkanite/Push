#include <sl.h>
#include "d3dkmt.h"


typedef struct NVAPI_PRIVATE_MEMORY_DATA
{
    BYTE Dummy[56];
	DWORD Free;
	BYTE Dummy2[16];

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


UINT16 OpenNvapi_GetMaximumEngineClock()
{

}


UINT16 OpenNvapi_GetMaximumMemoryClock()
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

}


UINT64 OpenNvapi_GetFreeMemory()
{
    NVAPI_PRIVATE_MEMORY_DATA memoryData = { 0 };

    memoryData.Dummy[0] = 0x41;
    memoryData.Dummy[1] = 0x44;
    memoryData.Dummy[2] = 0x56;
    memoryData.Dummy[3] = 0x4E;
    memoryData.Dummy[4] = 0x2;
    memoryData.Dummy[5] = 0x0;
    memoryData.Dummy[6] = 0x1;
    memoryData.Dummy[7] = 0x0;
    memoryData.Dummy[8] = 0x4C;
    memoryData.Dummy[9] = 0x0;
    memoryData.Dummy[10] = 0x0;
    memoryData.Dummy[11] = 0x0;
    memoryData.Dummy[12] = 0x2A;
    memoryData.Dummy[13] = 0x2A;
    memoryData.Dummy[14] = 0x56;
    memoryData.Dummy[15] = 0x4E;
    memoryData.Dummy[16] = 0x12;
    memoryData.Dummy[17] = 0x0;
    memoryData.Dummy[18] = 0x0;
    memoryData.Dummy[19] = 0x1;
	memoryData.Dummy2[15] = 0xB4;

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