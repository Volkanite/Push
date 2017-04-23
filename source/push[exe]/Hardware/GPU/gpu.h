#ifndef GPU_H
#define GPU_H

VOID GPU_Initialize(ULONG PciAddress);
VOID GPU_ForceMaximumClocks();

UINT16 GPU_GetMaximumEngineClock();
UINT16 GPU_GetMaximumMemoryClock();
UINT16 GPU_GetMaximumVoltage();
UINT16 GPU_GetEngineClock();
UINT16 GPU_GetMemoryClock();
UINT16 GPU_GetVoltage();
UINT16 GPU_GetFanSpeed();
UINT8 GPU_GetTemperature();
UINT8 GPU_GetLoad();
UINT64 GPU_GetTotalMemory();
UINT64 GPU_GetFreeMemory();


#endif //GPU_H