#include <sl.h>
#include <push.h>
#include <hardware.h>
#include <ring0.h>

#include "gpu.h"
#include "amdgpu.h"
#include "nvidiagpu.h"
#include "IntelGpu.h"


typedef struct _PCI_CONFIG
{
    WORD VendorId;
    WORD DeviceId;
}PCI_CONFIG;

WORD GPU_VendorId;


VOID GPU_Initialize( ULONG PciAddress )
{
    PCI_CONFIG pciConfig;

    R0ReadPciConfig(
        PciAddress,
        REGISTER_VENDORID,
        (BYTE *)&pciConfig,
        sizeof(pciConfig)
        );

    switch (pciConfig.VendorId)
    {
    case AMD:
        GPU_VendorId = AMD;
        AmdGpu_Initialize();
        break;
    case NVIDIA:
        GPU_VendorId = NVIDIA;
        break;
    case INTEL:
        GPU_VendorId = INTEL;
        break;
    default:
        break;
    }
}


UINT16 GPU_GetVoltage()
{
    switch (GPU_VendorId)
    {
    case AMD:
        return AmdGpu_GetVoltage();
        break;
    case NVIDIA:
        break;
    default:
        return 0;
        break;
    }

    return 0;
}


UINT16 GPU_GetEngineClock()
{
    switch (GPU_VendorId)
    {
    case AMD:
        return AmdGpu_GetEngineClock();
        break;
    case NVIDIA:
        break;
    default:
        return 0;
        break;
    }

    return 0;
}


UINT16 GPU_GetMemoryClock()
{
    switch (GPU_VendorId)
    {
    case AMD:
        return AmdGpu_GetMemoryClock();
        break;
    case NVIDIA:
        break;
    default:
        return 0;
        break;
    }

    return 0;
}


UINT64 GPU_GetTotalMemory()
{ 
    switch (GPU_VendorId)
    {
    case AMD:
        return AmdGpu_GetTotalMemory();
        break;
    }

    return 0;
}


UINT64 GPU_GetFreeMemory()
{ 
    switch (GPU_VendorId)
    {
    case AMD:
        return AmdGpu_GetFreeMemory();
        break;
    }

    return 0;
}


UINT16 GPU_GetMaximumEngineClock()
{ 
    return AmdGpu_GetEngineClockMax();
}


UINT16 GPU_GetMaximumMemoryClock()
{ 
    return AmdGpu_GetMemoryClockMax();
}


UINT16 GPU_GetMaximumVoltage()
{
    return AmdGpu_GetVoltageMax();
}


UINT16 GPU_GetFanSpeed(){ return 0; }
UINT8 GPU_GetTemperature(){ return 0; }
UINT8 GPU_GetLoad(){ return 0; }
VOID GPU_ForceMaximumClocks(){}