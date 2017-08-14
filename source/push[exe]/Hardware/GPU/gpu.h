#ifndef GPU_H
#define GPU_H

typedef struct _GPU_INFO
{
    int EngineClock;
    int MemoryClock;
    int Voltage;
    int Load;
    int Temperature;
    int FanSpeed;
    int FanDutyCycle;
    int MemoryUsed;
    int MemoryUsage;

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

VOID GPU_SetEngineClock( int Frequency );
VOID GPU_SetMemoryClock( int Frequency );
VOID GPU_SetVoltage( int Millivolts );

#endif //GPU_H