#include <sltypes.h>

#include "gpu.h"
#include "amdgpu.h"
#include "nvidiagpu.h"


#define NVIDIA  0x10DE
#define AMD     0x1002
#define INTEL   0x8086


CGPU* 
CreateGpuInterface( WORD VendorId )
{
	switch (VendorId)
	{
	case AMD:
		return new AmdGpu();
	case NVIDIA:
		return new NvidiaGpu();
	default:
		return NULL;
	}
}