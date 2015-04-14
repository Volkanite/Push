#include <sltypes.h>
#include "GenericGpu.h"
#include "d3dkmt.h"


UINT16 GenericGpu_GetEngineClock()
{
	return 0;
}


UINT16 GenericGpu_GetMemoryClock()
{
	return 0;
}


UINT64 GenericGpu_GetTotalMemory()
{
	return 0;
}


UINT64 GenericGpu_GetFreeMemory()
{
	return 0;
}


UINT8 GenericGpu_GetTemperature()
{
	return 0;
}


UINT8 GenericGpu_GetLoad()
{
	return D3DKMT_GetGpuUsage();
}


UINT16 GenericGpu_GetMaximumEngineClock()
{
	return 0;
}


UINT16 GenericGpu_GetMaximumMemoryClock()
{
	return 0;
}


VOID GenericGpu_ForceMaximumClocks()
{

}