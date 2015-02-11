#include <sl.h>


#define NVAPI_MAX_PHYSICAL_GPUS         64
#define NVAPI_MAX_USAGES_PER_GPU        33
#define NVAPI_MAX_PHYSICAL_GPUS         64
#define NVAPI_MAX_CLOCKS_PER_GPU        0x120
#define NVAPI_MAX_MEMORY_VALUES_PER_GPU 6


typedef struct _NV_CLOCKS
{
    UINT32 Version;
    UINT32 Clock[NVAPI_MAX_CLOCKS_PER_GPU];

}NV_CLOCKS;

typedef struct _NV_USAGES
{
    UINT32 Version;
    UINT32 Usage[NVAPI_MAX_USAGES_PER_GPU];

}NV_USAGES;


typedef INT32 *(*TYPE_NvAPI_QueryInterface)(UINT32 offset);
typedef INT32 (*TYPE_NvAPI_Initialize)();
typedef INT32 (*TYPE_NvAPI_EnumPhysicalGPUs)(INT32 **handles, INT32* count);
typedef INT32 (*TYPE_NvAPI_GetMemoryInfo)(INT32 *hPhysicalGpu, UINT32* memInfo);
typedef INT32 (*TYPE_NvAPI_GPU_GetUsages)(INT32 *handle, NV_USAGES* Usages);
typedef INT32 (*TYPE_NvAPI_GPU_GetAllClocks)(VOID* GpuHandle, NV_CLOCKS* Clocks);


TYPE_NvAPI_QueryInterface   NvAPI_QueryInterface    = NULL;
TYPE_NvAPI_Initialize       NvAPI_Initialize        = NULL;
TYPE_NvAPI_EnumPhysicalGPUs NvAPI_EnumPhysicalGPUs  = NULL;
TYPE_NvAPI_GetMemoryInfo    NvAPI_GetMemoryInfo     = NULL;
TYPE_NvAPI_GPU_GetUsages    NvAPI_GPU_GetUsages     = NULL;
TYPE_NvAPI_GPU_GetAllClocks NvAPI_GPU_GetAllClocks  = NULL;


UINT32 NvMemoryInfo[NVAPI_MAX_MEMORY_VALUES_PER_GPU] = { 0 };
INT32 *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
INT32 gpuCount = 0;



BOOLEAN
Nvapi_Initialize()
{
    VOID *nvapi = NULL;
    
    nvapi = SlLoadLibrary(L"nvapi.dll");

    if (!nvapi)
        return FALSE;

    // nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
    NvAPI_QueryInterface = (TYPE_NvAPI_QueryInterface) GetProcAddress(
        nvapi, 
        "nvapi_QueryInterface"
        );

    // some useful internal functions that aren't exported by nvapi.dll
    NvAPI_GetMemoryInfo     = (TYPE_NvAPI_GetMemoryInfo)    (*NvAPI_QueryInterface)(0x774AA982);
    NvAPI_Initialize        = (TYPE_NvAPI_Initialize)       (*NvAPI_QueryInterface)(0x0150E828);
    NvAPI_EnumPhysicalGPUs  = (TYPE_NvAPI_EnumPhysicalGPUs) (*NvAPI_QueryInterface)(0xE5AC921F);
    NvAPI_GPU_GetUsages     = (TYPE_NvAPI_GPU_GetUsages)    (*NvAPI_QueryInterface)(0x189A1FDF);
    NvAPI_GPU_GetAllClocks  = (TYPE_NvAPI_GPU_GetAllClocks) (*NvAPI_QueryInterface)(0x1BD69F49);

    NvMemoryInfo[0]  = (NVAPI_MAX_MEMORY_VALUES_PER_GPU * 4) | 0x20000;

    // initialize NvAPI library, call it once before calling any other NvAPI functions
    (*NvAPI_Initialize)();

    // get ur gpu handles, &gpucount"
    (*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);

    return TRUE;
}


UINT8
Nvapi_GetActivity()
{
    NV_USAGES usages;

    usages.Version = sizeof(NV_USAGES) | 0x10000;

    NvAPI_GPU_GetUsages(gpuHandles[0], &usages);

    return usages.Usage[2];
}


UINT16
Nvapi_GetEngineClock()
{
    NV_CLOCKS clocks;

    clocks.Version = sizeof(NV_CLOCKS) | 0x20000;
    
    NvAPI_GPU_GetAllClocks(gpuHandles[0], &clocks);

    return 0.001f * clocks.Clock[0];
}


UINT16
Nvapi_GetMemoryClock()
{
    NV_CLOCKS clocks;

    clocks.Version = sizeof(NV_CLOCKS) | 0x20000;
    
    NvAPI_GPU_GetAllClocks(gpuHandles[0], &clocks);

    return 0.001f * clocks.Clock[8];
}