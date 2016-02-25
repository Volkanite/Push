#include <sl.h>
#include <push.h>

#include "gpu.h"
#include "amdgpu.h"
#include "nvidiagpu.h"
#include "IntelGpu.h"
#include "GenericGpu.h"


#define NVIDIA  0x10DE
#define AMD     0x1002
#define INTEL   0x8086


GPU_ADAPTER* CreateGpuAdapter( WORD VendorId )
{
    GPU_ADAPTER *gpuAdapter = (GPU_ADAPTER*) RtlAllocateHeap(
        PushHeapHandle, 
        HEAP_ZERO_MEMORY,
        sizeof(GPU_ADAPTER)
        );

    switch (VendorId)
    {
    case AMD:
        AmdGpu_CreateAdapter(gpuAdapter);
        break;
    case NVIDIA:
        NvidiaGpu_CreateAdapter(gpuAdapter);
        break;
    case INTEL:
        IntelGpu_CreateAdapter(gpuAdapter);
        break;
    default:
        GenericGpu_CreateAdapter(gpuAdapter);
        break;
    }

    return gpuAdapter;
}