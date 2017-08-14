#include <sl.h>
#include <push.h>

#include "NvidiaGpu.h"
#include <hardware.h>
#include "NvThermalDiode\NvThermalDiode.h"
#include "nvapi.h"
#include "OpenNvapi.h"
#include "d3dkmt.h"


BYTE GfCoreFamily = 0;
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

    GfCoreFamily = (ReadGpuRegister(0) >> 20) & 0xff;
    //m_dwDiodeGainMul = GetDiodeGainMul(GfCoreFamily);
    GfInitialized = TRUE;

    NvtdInitialize();

    return TRUE;
}


static int CalcSpeed_nv50(int base_freq, int m1, int m2, int n1, int n2, int p)
{
    return (int)((float)(n1*n2) / (m1*m2) * base_freq) >> p;
}

float GetClock_nv50(int base_freq, unsigned int pll, unsigned int pll2)
{
    int m1, m2, n1, n2, p;

    p = (pll >> 16) & 0x03;
    m1 = pll2 & 0xFF;
    n1 = (pll2 >> 8) & 0xFF;

    /* This is always 1 for NV5x? */
    m2 = 1;
    n2 = 1;

    /* The clocks need to be multiplied by 4 for some reason. Is this 4 stored in 0x4000/0x4004? */
    return (float)4 * CalcSpeed_nv50(base_freq, m1, m2, n1, n2, p) / 1000;
}


float nv50_get_gpu_speed()
{
    int pll = ReadGpuRegister(0x4028);
    int pll2 = ReadGpuRegister(0x402c);
    int base_freq = 25000;

    return (float)GetClock_nv50(base_freq, pll, pll2);
}


float nv50_get_memory_speed()
{
    int pll = ReadGpuRegister(0x4008);
    int pll2 = ReadGpuRegister(0x400c);
    int base_freq = 27000;

    return (float)GetClock_nv50(base_freq, pll, pll2);
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

    Nvapi_Initialize();
    D3DKMTInitialize();
}


UINT16 NvidiaGpu_GetEngineClock()
{
    return nv50_get_gpu_speed();
}


UINT16 NvidiaGpu_GetMemoryClock()
{
    return nv50_get_memory_speed();
}


UINT64 NvidiaGpu_GetTotalMemory()
{
    return ReadGpuRegister(0x10020c);
}


UINT64 NvidiaGpu_GetFreeMemory()
{
    return OpenNvapi_GetFreeMemory();
}


UINT8 NvidiaGpu_GetTemperature()
{
    if (!InitGeForce())
        return 0;

    return NvtdGetTemperature();
}


UINT8 NvidiaGpu_GetLoad()
{
    return Nvapi_GetActivity();
}


UINT16 NvidiaGpu_GetEngineClockMax()
{
    return Nvapi_GetMaxEngineClock();
}


UINT16 NvidiaGpu_GetMaxMemoryClock()
{
    return Nvapi_GetMaxMemoryClock();
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