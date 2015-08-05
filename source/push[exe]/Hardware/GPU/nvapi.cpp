#include <sl.h>
#include "nvapi.h"


TYPE_NvAPI_QueryInterface           NvAPI_QueryInterface;
TYPE_NvAPI_Initialize               NvAPI_Initialize;
TYPE_NvAPI_EnumPhysicalGPUs         NvAPI_EnumPhysicalGPUs;
TYPE_NvAPI_GetMemoryInfo            NvAPI_GetMemoryInfo;
TYPE_NvAPI_GPU_GetUsages            NvAPI_GPU_GetUsages;
TYPE_NvAPI_GPU_GetAllClocks         NvAPI_GPU_GetAllClocks;
TYPE_NvAPI_GPU_GetPstatesInfo       NvAPI_GPU_GetPstatesInfo;
TYPE_NvAPI_GPU_GetThermalSettings   NvAPI_GPU_GetThermalSettings;


INT32 *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
INT32 gpuCount = 0;
INT32 *displayHandles;


BOOLEAN Nvapi_Initialize()
{
    VOID *nvapi = NULL;

    nvapi = Module::Load(L"nvapi.dll");

    if (!nvapi) return FALSE;

    NvAPI_QueryInterface = (TYPE_NvAPI_QueryInterface) Module::GetProcedureAddress(
        nvapi,
        "nvapi_QueryInterface"
        );

    NvAPI_Initialize             =  (TYPE_NvAPI_Initialize) NvAPI_QueryInterface(0x0150E828);
    NvAPI_EnumPhysicalGPUs       =  (TYPE_NvAPI_EnumPhysicalGPUs) NvAPI_QueryInterface(0xE5AC921F);
    NvAPI_GPU_GetUsages          =  (TYPE_NvAPI_GPU_GetUsages) NvAPI_QueryInterface(0x189A1FDF);
    NvAPI_GPU_GetAllClocks       =  (TYPE_NvAPI_GPU_GetAllClocks) NvAPI_QueryInterface(0x1BD69F49);
    NvAPI_GPU_GetPstatesInfo     =  (TYPE_NvAPI_GPU_GetPstatesInfo) NvAPI_QueryInterface(0xBA94C56E);
    NvAPI_GetMemoryInfo          =  (TYPE_NvAPI_GetMemoryInfo) NvAPI_QueryInterface(0x774AA982);
    NvAPI_GPU_GetThermalSettings =  (TYPE_NvAPI_GPU_GetThermalSettings) NvAPI_QueryInterface(0xE3640A56);

    NvAPI_Initialize();
    NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);

    return TRUE;
}


UINT8 Nvapi_GetActivity()
{
    NV_USAGES usages;

    usages.Version = sizeof(NV_USAGES) | 0x10000;

    NvAPI_GPU_GetUsages(gpuHandles[0], &usages);

    return usages.Usage[NVAPI_USAGE_TARGET_GPU];
}


UINT16 Nvapi_GetEngineClock()
{
    NV_CLOCKS clocks;

    clocks.Version = sizeof(NV_CLOCKS) | 0x20000;

    NvAPI_GPU_GetAllClocks(gpuHandles[0], &clocks);

    return 0.001f * clocks.Clock[NVAPI_CLOCK_TARGET_ENGINE];
}


UINT16 Nvapi_GetMemoryClock()
{
    NV_CLOCKS clocks;

    clocks.Version = sizeof(NV_CLOCKS) | 0x20000;

    NvAPI_GPU_GetAllClocks(gpuHandles[0], &clocks);

    return 0.001f * clocks.Clock[NVAPI_CLOCK_TARGET_MEMORY];
}


UINT16 Nvapi_GetMaxEngineClock()
{
    NV_GPU_PERF_PSTATES_INFO pstateInfo = {0};

    pstateInfo.Version = sizeof(NV_GPU_PERF_PSTATES_INFO) | 0x10000;

    NvAPI_GPU_GetPstatesInfo(gpuHandles[0], &pstateInfo);

    return pstateInfo.Pstates[NVAPI_GPU_PERF_PSTATE_P0].Clocks[0].Freq * 0.001f;
}


UINT16 Nvapi_GetMaxMemoryClock()
{
    NV_GPU_PERF_PSTATES_INFO pstateInfo = {0};

    pstateInfo.Version = sizeof(NV_GPU_PERF_PSTATES_INFO) | 0x10000;

    NvAPI_GPU_GetPstatesInfo(gpuHandles[0], &pstateInfo);

    return pstateInfo.Pstates[NVAPI_GPU_PERF_PSTATE_P0].Clocks[1].Freq * 0.001f;
}


UINT64 Nvapi_GetTotalMemory()
{
    NV_MEMORY_INFO memoryInfo = {0};

    memoryInfo.Version = sizeof(NV_MEMORY_INFO) | 0x20000;

    NvAPI_GetMemoryInfo(displayHandles, &memoryInfo);

    return memoryInfo.Value[0] * 1024; //kilobytes -> bytes
}


UINT64 Nvapi_GetFreeMemory()
{
    NV_MEMORY_INFO memoryInfo = {0};

    memoryInfo.Version = sizeof(NV_MEMORY_INFO) | 0x20000;

    NvAPI_GetMemoryInfo(displayHandles, &memoryInfo);

    return memoryInfo.Value[4] * 1024; //kilobytes -> bytes
}


UINT8 Nvapi_GetTemperature()
{
    NV_GPU_THERMAL_SETTINGS thermalInfo = {0};

    thermalInfo.Version = sizeof(NV_GPU_THERMAL_SETTINGS) | 0x20000;
    thermalInfo.Count = 0;
    thermalInfo.sensor[0].Controller = NVAPI_THERMAL_CONTROLLER_UNKNOWN;
    thermalInfo.sensor[0].Target = NVAPI_THERMAL_TARGET_GPU;

    NvAPI_GPU_GetThermalSettings(gpuHandles[0], 0, &thermalInfo);

    return thermalInfo.sensor[0].CurrentTemp;
}


VOID Nvapi_ForceMaximumClocks()
{

}
