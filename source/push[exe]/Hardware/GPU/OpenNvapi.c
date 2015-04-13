#include <sl.h>
#include "d3dkmt.h"


typedef struct NVAPI_PRIVATE_MEMORY_DATA
{
    BYTE Dummy[76];
    //NV_MEMORY_INFO MemoryInformation;

}NVAPI_PRIVATE_MEMORY_DATA;


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
    NVAPI_PRIVATE_MEMORY_DATA privData = { 0 };

    privData.Dummy[0] = 0x41;
    privData.Dummy[1] = 0x44;
    privData.Dummy[2] = 0x56;
    privData.Dummy[3] = 0x4E;
    privData.Dummy[4] = 0x2;
    privData.Dummy[5] = 0x0;
    privData.Dummy[6] = 0x1;
    privData.Dummy[7] = 0x0;
    privData.Dummy[8] = 0x4C;
    privData.Dummy[9] = 0x0;
    privData.Dummy[10] = 0x0;
    privData.Dummy[11] = 0x0;
    privData.Dummy[12] = 0x2A;
    privData.Dummy[13] = 0x2A;
    privData.Dummy[14] = 0x56;
    privData.Dummy[15] = 0x4E;
    privData.Dummy[16] = 0x12;
    privData.Dummy[17] = 0x0;
    privData.Dummy[18] = 0x0;
    privData.Dummy[19] = 0x1;
    privData.Dummy[75] = 0xB4;
    
    D3DKMT_GetPrivateDriverData(
        &privData, 
        sizeof(NVAPI_PRIVATE_MEMORY_DATA)
        );
    
    VOID *adr = &privData.Dummy[0x38];
    DWORD *kb = (DWORD*)adr;
    return *kb * 1024; //kilobytes -> bytes
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