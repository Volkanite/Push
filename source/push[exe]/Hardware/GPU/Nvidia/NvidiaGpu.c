#include <push.h>

#include "NvidiaGpu.h"
#include "NvThermalDiode\NvThermalDiode.h"
#include "nvapi.h"
#include "OpenNvapi.h"
#include "..\d3dkmt.h"
#include "nva8.h"
#include "nv50.h"


BYTE CoreFamily;
LONG    m_dwDiodeGainMul;
LONG GetDiodeGainMul( DWORD coreFamily );
BOOLEAN GfInitialized;

UINT8 NvidiaGpu_GetActivity();
UINT16 NvidiaGpu_GetEngineClock();
UINT16 NvidiaGpu_GetMemoryClock();
UINT16 NvidiaGpu_GetMaxEngineClock();
UINT16 NvidiaGpu_GetMaxMemoryClock();
UINT64 NvidiaGpu_GetTotalMemory();
UINT64 NvidiaGpu_GetFreeMemory();
UINT8  NvidiaGpu_GetTemperature();
VOID NvidiaGpu_ForceMaximumClocks();
UINT8 NvidiaGpu_GetLoad();
UINT16 NvidiaGpu_GetVoltage();
UINT16 NvidiaGpu_GetFanSpeed();

BOOLEAN
InitGeForce()
{
    if (GfInitialized)
        return TRUE;

    CoreFamily = (ReadGpuRegister(0) >> 20) & 0xff;

    GfInitialized = TRUE;

    NvtdInitialize();

    return TRUE;
}


typedef enum _GPU_INTERFACE
{
    GPU_INTERFACE_NVAPI,
    GPU_INTERFACE_OPEN_NVAPI,
    GPU_INTERFACE_D3DKMT,
    GPU_INTERFACE_PURE,

} GPU_INTERFACE;


VOID NvidiaGpu_Initialize()
{
    GPU_INTERFACE gpuInterface = GPU_INTERFACE_PURE;
    
    InitGeForce();
    Nvapi_Initialize();
    D3DKMTInitialize();
}


UINT16 NvidiaGpu_GetEngineClock()
{
    switch (CoreFamily)
    {
    case 0xA8:
        return nva8_get_core_clock();
    case 0x50:
        return nv50_get_core_clock();
	case 0xE4:
    default:
		return Nvapi_GetEngineClock();
    }
}


UINT16 NvidiaGpu_GetMemoryClock()
{
    switch (CoreFamily)
    {
    case 0xA8:
        return nva8_get_memory_clock();
    case 0x50:
        return nv50_get_memory_clock();
	case 0xE4:
    default:
        return Nvapi_GetMemoryClock();
    }
}


UINT8 NvidiaGpu_GetTemperature()
{
    //return ReadGpuRegister(0x20400);
	return Nvapi_GetTemperature();
}


UINT64 NvidiaGpu_GetTotalMemory()
{
    //return ReadGpuRegister(0x10020c);
	return Nvapi_GetTotalMemory();
}


UINT64 NvidiaGpu_GetFreeMemory()
{
    //return OpenNvapi_GetFreeMemory();
	return Nvapi_GetFreeMemory();
}


UINT8 NvidiaGpu_GetLoad()
{
    return Nvapi_GetLoad();
}


UINT16 NvidiaGpu_GetEngineClockMax()
{
    return Nvapi_GetBaseEngineClock();
}


UINT16 NvidiaGpu_GetMemoryClockMax()
{
    return Nvapi_GetBaseMemoryClock();
}


VOID NvidiaGpu_ForceMaximumClocks()
{

}


UINT16 NvidiaGpu_GetVoltage()
{
    return Nvapi_GetVoltage();
}


UINT16 NvidiaGpu_GetFanSpeed()
{
    return Nvapi_GetFanSpeed();
}


UINT16 NvidiaGpu_GetFanDutyCycle()
{
	return Nvapi_GetFanDutyCycle();
}


VOID NvidiaGpu_SetFanDutyCycle( int DutyCycle )
{
	Nvapi_SetFanDutyCycle(DutyCycle);
}
