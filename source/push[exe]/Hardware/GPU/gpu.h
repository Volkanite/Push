#ifndef GPU_H
#define GPU_H

typedef struct _GPU_INFO
{
    int EngineClock;
    int MemoryClock;
    int Voltage;
    int ActivityPercent;

}GPU_INFO;

VOID GPU_Initialize(ULONG PciAddress);
VOID GPU_ForceMaximumClocks();
VOID GPU_GetInfo(GPU_INFO* Info);

UINT16 GPU_GetMaximumEngineClock();
UINT16 GPU_GetMaximumMemoryClock();
UINT16 GPU_GetMaximumVoltage();
UINT16 GPU_GetFanSpeed();
UINT8 GPU_GetTemperature();
UINT8 GPU_GetLoad();
UINT64 GPU_GetTotalMemory();
UINT64 GPU_GetFreeMemory();


#endif //GPU_H