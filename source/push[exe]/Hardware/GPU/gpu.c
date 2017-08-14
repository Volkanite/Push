#include <sl.h>
#include <push.h>
#include <hardware.h>
#include <ring0.h>

#include "gpu.h"
#include "AMD\AmdGpu.h"
#include "Nvidia\NvidiaGpu.h"
#include "Intel\IntelGpu.h"
#include "GPU\d3dkmt.h"


typedef struct _PCI_CONFIG
{
    WORD VendorId;
    WORD DeviceId;
}PCI_CONFIG;

WORD GPU_VendorId;


VOID GPU_Initialize( ULONG PciAddress )
{
    PCI_CONFIG pciConfig;
    BOOLEAN result;
    DWORD vendorID;

    result = R0ReadPciConfig(
        PciAddress, 
        REGISTER_VENDORID,
        (BYTE *)&pciConfig,
        sizeof(pciConfig)
        );

    vendorID = pciConfig.VendorId;

    if (!result)
    {
        wchar_t devicePath[260];

        GetDisplayAdapterDevicePath(devicePath);
        swscanf_s(devicePath, L"\\\\?\\pci#ven_%04x", &vendorID);
    }

    switch (vendorID)
    {
    case AMD:
        GPU_VendorId = AMD;
        AmdGpu_Initialize();
        break;
    case NVIDIA:
        GPU_VendorId = NVIDIA;
        NvidiaGpu_Initialize();
        break;
    case INTEL:
        GPU_VendorId = INTEL;
        break;
    default:
        break;
    }
}


VOID GPU_GetInfo( GPU_INFO* Info )
{
    UINT64 total, free, used;

    switch (GPU_VendorId)
    {
    case AMD:
        AmdGpu_GetInfo(Info);

        //AMD ADL gpu load is finicky
        Info->Load = D3DKMT_GetGpuUsage();

        total = GPU_GetTotalMemory();
        free = GPU_GetFreeMemory();
        used = total - free;

        Info->MemoryUsed = used / 1048576;
        Info->MemoryUsage = 100 * ((float)used / (float)total);
        break;
    case NVIDIA:
        Info->EngineClock = NvidiaGpu_GetEngineClock();
		Info->MemoryClock = NvidiaGpu_GetMemoryClock();
        Info->Load = NvidiaGpu_GetLoad();
        Info->Temperature = NvidiaGpu_GetTemperature();
        Info->FanSpeed = 0;
        Info->FanDutyCycle = NvidiaGpu_GetFanSpeed();
        break;
    default:
        return;
        break;
    }
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
    switch (GPU_VendorId)
    {
    case AMD:
        return AmdGpu_GetEngineClockMax();
    case NVIDIA:
        return NvidiaGpu_GetEngineClockMax();
    default:
        return 0;
    }
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


VOID GPU_ForceMaximumClocks()
{
    AmdGpu_ForceMaximumClocks();
}