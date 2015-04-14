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


GPU_ADAPTER* CreateGpuInterface( WORD VendorId )
{
	GPU_ADAPTER *gpuAdapter = (GPU_ADAPTER*) RtlAllocateHeap(PushHeapHandle, 0, sizeof(GPU_ADAPTER));

	switch (VendorId)
	{
	case AMD:
		AmdGpu_CreateInterface(gpuAdapter);
		break;
	case NVIDIA:
		NvidiaGpu_CreateInterface(gpuAdapter);
		break;
	case INTEL:
		IntelGpu_CreateInterface(gpuAdapter);
		break;
	default:
		GenericGpu_CreateInterface(gpuAdapter);
		break;
	}

	return gpuAdapter;
}