#include "gpu.h"

VOID AmdGpu_Initialize();

UINT16 AmdGpu_GetEngineClock();
UINT16 AmdGpu_GetMemoryClock();
UINT16 AmdGpu_GetVoltage();
UINT16 AmdGpu_GetEngineClockMax();
UINT16 AmdGpu_GetMemoryClockMax();
UINT16 AmdGpu_GetVoltageMax();
UINT64 AmdGpu_GetTotalMemory();
UINT64 AmdGpu_GetFreeMemory();