#include <sl.h>
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
    //m_dwDiodeGainMul = GetDiodeGainMul(GfCoreFamily);
    GfInitialized = TRUE;

    NvtdInitialize();

    return TRUE;
}


/*//////////////////////////////////////////////////////////////////////
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
}*/



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

	/*if (nv_card->debug)
		printf("m1=%d m2=%d n1=%d n2=%d p=%d\n", m1, m2, n1, n2, p);*/

	/* The clocks need to be multiplied by 4 for some reason. Is this 4 stored in 0x4000/0x4004? */
	return (float)4 * CalcSpeed_nv50(base_freq, m1, m2, n1, n2, p) / 1000;
}
static float nv50_get_gpu_speed()
{
	int pll = ReadGpuRegister(0x4024);
	int pll2 = ReadGpuRegister(0x402c);
	int base_freq = 27000;

	return (float)GetClock_nv50(base_freq, pll, pll2);
}


NvidiaGpu::NvidiaGpu()
{
	Nvapi_Initialize();
}


UINT16 
NvidiaGpu::GetEngineClock()
{
	return nv50_get_gpu_speed();
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