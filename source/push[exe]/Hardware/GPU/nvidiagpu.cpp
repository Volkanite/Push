#include <sl.h>
#include <push.h>

#include "NvidiaGpu.h"
#include <hwinfo.h>
#include "NvThermalDiode\NvThermalDiode.h"
#include "nvapi.h"
#include "OpenNvapi.h"
#include "d3dkmt.h"


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


NvidiaGpu::NvidiaGpu()
{
    Nvapi_Initialize();
    D3DKMTInitialize();
}


UINT16 NvidiaGpu::GetEngineClock()
{
    return nv50_get_gpu_speed();
}


UINT16 NvidiaGpu::GetMemoryClock()
{
    return nv50_get_memory_speed();
}


UINT64 NvidiaGpu::GetTotalMemory()
{
    return ReadGpuRegister(0x10020c);
}

#define NVAPI_MAX_MEMORY_VALUES_PER_GPU 5
typedef struct _NV_MEMORY_INFO
{
    UINT32 Version;
    UINT32 Value[NVAPI_MAX_MEMORY_VALUES_PER_GPU];

}NV_MEMORY_INFO;
typedef struct _NVAPI_PRIVATE_DATA
{
    BYTE Dummy[76];
    //NV_MEMORY_INFO MemoryInformation;

}NVAPI_PRIVATE_DATA;


UINT64 NvidiaGpu::GetFreeMemory()
{
    return OpenNvapi_GetFreeMemory();
}


UINT8 
NvidiaGpu::GetTemperature()
{
    if (!InitGeForce())
        return 0;

    return NvtdGetTemperature();
}


UINT8 NvidiaGpu::GetLoad()
{
    return Nvapi_GetActivity();
}


UINT16 NvidiaGpu::GetMaximumEngineClock()
{
    return Nvapi_GetMaxEngineClock();
}


UINT16 NvidiaGpu::GetMaximumMemoryClock()
{
    return Nvapi_GetMaxMemoryClock();
}


VOID 
NvidiaGpu::ForceMaximumClocks()
{

}