#include "..\gpu.h"

VOID NvidiaGpu_Initialize();
UINT16 NvidiaGpu_GetEngineClock();
UINT16 NvidiaGpu_GetMemoryClock();
UINT16 NvidiaGpu_GetEngineClockMax();
UINT16 NvidiaGpu_GetMemoryClockMax();
UINT8 NvidiaGpu_GetTemperature();
UINT8 NvidiaGpu_GetLoad();
UINT16 NvidiaGpu_GetFanSpeed();
UINT64 NvidiaGpu_GetTotalMemory();
UINT64 NvidiaGpu_GetFreeMemory();
UINT16 NvidiaGpu_GetVoltage();
UINT16 NvidiaGpu_GetVoltageMax();