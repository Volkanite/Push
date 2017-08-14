#ifndef AMD_GPU_H
#define AMD_GPU_H

#include "..\gpu.h"



VOID AmdGpu_Initialize();
VOID AmdGpu_GetInfo();
VOID AmdGpu_ForceMaximumClocks();

UINT16 AmdGpu_GetEngineClockMax();
UINT16 AmdGpu_GetMemoryClockMax();
UINT16 AmdGpu_GetVoltageMax();
UINT64 AmdGpu_GetTotalMemory();
UINT64 AmdGpu_GetFreeMemory();

#include "adl.h"

#endif //AMD_GPU_H