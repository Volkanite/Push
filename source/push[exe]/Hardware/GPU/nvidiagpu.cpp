#include <sltypes.h>
#include <slntuapi.h>
#include <slmodule.h>
#include <pushbase.h>
#include <push.h>

#include "NvidiaGpu.h"
#include <hwinfo.h>
#include "NvThermalDiode\NvThermalDiode.h"

BYTE GfCoreFamily = 0;
LONG    m_dwDiodeGainMul;
LONG GetDiodeGainMul( DWORD coreFamily );
    #define NVAPI_MAX_PHYSICAL_GPUS 64
//INT32* gpuHandles[NVAPI_MAX_PHYSICAL_GPUS];
#define NVAPI_MAX_USAGES_PER_GPU        34
#define NVAPI_MAX_MEMORY_VALUES_PER_GPU 6
#define NVAPI_MAX_PHYSICAL_GPUS 64
#define NVAPI_MAX_USAGES_PER_GPU 34
DWORD gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };
DWORD mem_info_values[NVAPI_MAX_MEMORY_VALUES_PER_GPU] = { 0 };
INT32 *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
INT32 *displayHandles;
INT32 gpuCount = 0;
BOOLEAN GfNvapiInitialized;
BOOLEAN GfInitialized;


typedef INT32 *(*TYPE_NvAPI_QueryInterface)(UINT32 offset);
typedef INT32 (*TYPE_NvAPI_Initialize)();
typedef INT32 (*TYPE_NvAPI_EnumPhysicalGPUs)(INT32 **handles, INT32* count);
typedef INT32 (*TYPE_NvAPI_GetMemoryInfo)(INT32 *hPhysicalGpu, UINT32* memInfo);
typedef INT32 (*TYPE_NvAPI_GPU_GetUsages)(INT32 *handle, UINT32* usages);

TYPE_NvAPI_QueryInterface   NvAPI_QueryInterface    = NULL;
TYPE_NvAPI_Initialize       NvAPI_Initialize        = NULL;
TYPE_NvAPI_EnumPhysicalGPUs NvAPI_EnumPhysicalGPUs  = NULL;
TYPE_NvAPI_GetMemoryInfo    NvAPI_GetMemoryInfo     = NULL;
TYPE_NvAPI_GPU_GetUsages    NvAPI_GPU_GetUsages     = NULL;


BOOLEAN
InitNvapi()
{
    VOID *nvapi = NULL;

    if (GfNvapiInitialized)
        return TRUE;

    nvapi = SlLoadLibrary(L"nvapi.dll");

    if (!nvapi)
        return FALSE;

    // nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
    NvAPI_QueryInterface = (TYPE_NvAPI_QueryInterface) GetProcAddress(nvapi, "nvapi_QueryInterface");

    // some useful internal functions that aren't exported by nvapi.dll
    NvAPI_GetMemoryInfo     = (TYPE_NvAPI_GetMemoryInfo)    (*NvAPI_QueryInterface)(0x774AA982);
    NvAPI_Initialize        = (TYPE_NvAPI_Initialize)       (*NvAPI_QueryInterface)(0x0150E828);
    NvAPI_EnumPhysicalGPUs  = (TYPE_NvAPI_EnumPhysicalGPUs) (*NvAPI_QueryInterface)(0xE5AC921F);
    NvAPI_GPU_GetUsages     = (TYPE_NvAPI_GPU_GetUsages)    (*NvAPI_QueryInterface)(0x189A1FDF);

    //gpu usages
    gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

    // initialize NvAPI library, call it once before calling any other NvAPI functions
    (*NvAPI_Initialize)();

    mem_info_values[0]  = (NVAPI_MAX_MEMORY_VALUES_PER_GPU * 4) | 0x20000;
    // get ur gpu handles, &gpucount"
    (*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);

    GfNvapiInitialized = TRUE;

    return TRUE;
}


BOOLEAN
InitGeForce()
{
    if (GfInitialized)
        return TRUE;

    GfCoreFamily = (ReadGpuRegister(0) >> 20) & 0xff;
    m_dwDiodeGainMul = GetDiodeGainMul(GfCoreFamily);
    GfInitialized = TRUE;

    NvtdInitialize();

    return TRUE;
}


//////////////////////////////////////////////////////////////////////
// Get default thermal diode gain mul calibration parameter for
// thermal diode capable GPUs
//////////////////////////////////////////////////////////////////////
LONG
GetDiodeGainMul( DWORD coreFamily )
{
    switch (coreFamily)
    {
    case 0x43:
        //NV43
        return 792;
        break;
    case 0x44:
    case 0x4A:
    case 0x47:
        //NV44 and G70
        return 780;
        break;
    case 0x46:
    case 0x49:
    case 0x4B:
        //G71, G72 and G73
        return 450;
        break;
    case 0x50:
        //G80
        return 430;
        break;
    case 0x84:
    case 0x86:
    case 0x94:
        //G84, G86 and G94
        return 1;
        break;
    case 0x92:
        //G92
        return 10;
        break;
    default:
        //return 0 if GPU has no on-die thermal diode
        return 0;
    }
}


UINT16 
NvidiaGpu::GetEngineClock()
{
    return 0;
}


UINT16 
NvidiaGpu::GetMemoryClock()
{
    return 0;
}


UINT64 
NvidiaGpu::GetTotalMemory()
{
    return 0;
}


UINT64 
NvidiaGpu::GetFreeMemory()
{
    return 0;
}


UINT8 
NvidiaGpu::GetTemperature()
{
    if (!InitGeForce())
        return 0;

    return NvtdGetTemperature();
}


UINT8 
NvidiaGpu::GetLoad()
{
     if (!InitNvapi())
        return 0;

    (*NvAPI_GPU_GetUsages)(gpuHandles[0], gpuUsages);

    return gpuUsages[3];
}


UINT16 
NvidiaGpu::GetMaximumEngineClock()
{
    return 0;
}


UINT16 
NvidiaGpu::GetMaximumMemoryClock()
{
    return 0;
}


VOID 
NvidiaGpu::ForceMaximumClocks()
{

}