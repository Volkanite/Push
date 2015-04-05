#include <sltypes.h>
#include <slntuapi.h>
#include <slmodule.h>
#include <pushbase.h>
#include <push.h>

#include "NvidiaGpu.h"
#include <hwinfo.h>
#include "NvThermalDiode\NvThermalDiode.h"
#include "nvapi.h"


BYTE GfCoreFamily = 0;
LONG    m_dwDiodeGainMul;
LONG GetDiodeGainMul( DWORD coreFamily );

INT32 *displayHandles;
BOOLEAN GfInitialized;


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


NvidiaGpu::NvidiaGpu()
{
    Nvapi_Initialize();
}


UINT16 
NvidiaGpu::GetEngineClock()
{
    return Nvapi_GetEngineClock();
}


UINT16 
NvidiaGpu::GetMemoryClock()
{
    return Nvapi_GetMemoryClock();
}


UINT64 NvidiaGpu::GetTotalMemory()
{
    return Nvapi_GetTotalMemory();
}


UINT64 NvidiaGpu::GetFreeMemory()
{
    return Nvapi_GetFreeMemory();
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
    return Nvapi_GetActivity();
}


UINT16 
NvidiaGpu::GetMaximumEngineClock()
{
    return Nvapi_GetMaxEngineClock();
}


UINT16 
NvidiaGpu::GetMaximumMemoryClock()
{
    return Nvapi_GetMaxMemoryClock();
}


VOID 
NvidiaGpu::ForceMaximumClocks()
{

}