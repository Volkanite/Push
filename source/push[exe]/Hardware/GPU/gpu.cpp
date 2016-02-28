#include <sl.h>
#include <push.h>
#include <hardware.h>
#include <ring0.h>

#include "gpu.h"
#include "amdgpu.h"
#include "nvidiagpu.h"
#include "IntelGpu.h"
#include "GenericGpu.h"





typedef struct _PCI_CONFIG
{
    WORD VendorId;
    WORD DeviceId;
}PCI_CONFIG;


GPU_ADAPTER* CreateGpuAdapter( ULONG PciAddress )
{
    PCI_CONFIG pciConfig;

    GPU_ADAPTER *gpuAdapter = (GPU_ADAPTER*) RtlAllocateHeap(
        PushHeapHandle, 
        HEAP_ZERO_MEMORY,
        sizeof(GPU_ADAPTER)
        );

    R0ReadPciConfig(
        PciAddress,
        REGISTER_VENDORID,
        (BYTE *)&pciConfig,
        sizeof(pciConfig)
        );

    gpuAdapter->DeviceId = pciConfig.DeviceId;

    switch (pciConfig.VendorId)
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