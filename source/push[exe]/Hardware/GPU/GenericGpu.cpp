#include <sltypes.h>
#include "GenericGpu.h"
#include "d3dkmt.h"


UINT16 
GenericGpu::GetEngineClock()
{
	return 0;
}


UINT16 
GenericGpu::GetMemoryClock()
{
	return 0;
}


UINT64 
GenericGpu::GetTotalMemory()
{
	return 0;
}


UINT64 
GenericGpu::GetFreeMemory()
{
	return 0;
}


UINT8 
GenericGpu::GetTemperature()
{
	return 0;
}


UINT8 
GenericGpu::GetLoad()
{
	return D3DKMTGetGpuUsage();
}


UINT16 
GenericGpu::GetMaximumEngineClock()
{
	return 0;
}


UINT16 
GenericGpu::GetMaximumMemoryClock()
{
	return 0;
}


VOID 
GenericGpu::ForceMaximumClocks()
{

}