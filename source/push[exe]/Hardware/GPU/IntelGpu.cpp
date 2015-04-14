#include <sltypes.h>
#include "IntelGpu.h"
#include "d3dkmt.h"
#include "..\CPU\intel.h"


UINT16 IntelGpu_GetEngineClock()
{
	return 0;
}


UINT16 IntelGpu_GetMemoryClock()
{
	return 0;
}


UINT64 IntelGpu_GetTotalMemory()
{
	return 0;
}


UINT64 IntelGpu_GetFreeMemory()
{
	return 0;
}


UINT8 IntelGpu_GetTemperature()
{
	return GetIntelTemp();
}


UINT8 IntelGpu_GetLoad()
{
	return D3DKMT_GetGpuUsage();
}


UINT16 IntelGpu_GetMaximumEngineClock()
{
	return 0;
}


UINT16 IntelGpu_GetMaximumMemoryClock()
{
	return 0;
}


VOID IntelGpu_ForceMaximumClocks()
{

}