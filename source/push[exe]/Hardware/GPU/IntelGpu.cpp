#include <sltypes.h>
#include "IntelGpu.h"
#include "d3dkmt.h"
#include "..\CPU\intel.h"


UINT16 
IntelGpu::GetEngineClock()
{
	return 0;
}


UINT16 
IntelGpu::GetMemoryClock()
{
	return 0;
}


UINT64 
IntelGpu::GetTotalMemory()
{
	return 0;
}


UINT64 
IntelGpu::GetFreeMemory()
{
	return 0;
}


UINT8 
IntelGpu::GetTemperature()
{
	return GetIntelTemp();
}


UINT8 
IntelGpu::GetLoad()
{
	return D3DKMTGetGpuUsage();
}


UINT16 
IntelGpu::GetMaximumEngineClock()
{
	return 0;
}


UINT16 
IntelGpu::GetMaximumMemoryClock()
{
	return 0;
}


VOID 
IntelGpu::ForceMaximumClocks()
{

}