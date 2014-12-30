#include "gpu.h"


class IntelGpu : public CGPU{
public:
	UINT16 GetEngineClock();
	UINT16 GetMemoryClock();
	UINT64 GetTotalMemory();
	UINT64 GetFreeMemory();
	UINT8 GetTemperature();
	UINT8 GetLoad();
	UINT16 GetMaximumEngineClock();
	UINT16 GetMaximumMemoryClock();
	VOID ForceMaximumClocks();
};