#include "gpu.h"


class NvidiaGpu : public CGPU{
public:
    NvidiaGpu();
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