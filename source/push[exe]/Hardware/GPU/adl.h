#include "AmdGpu.h"

VOID Adl_Initialize();
VOID Adl_GetInfo(GPU_INFO* Activity);

UINT16 Adl_GetEngineClockMax();
UINT16 Adl_GetMemoryClockMax();
UINT16 Adl_GetVoltageMax();
UINT8 Adl_GetTemperature();
UINT32 Adl_GetFanSpeed();

VOID Adl_SetEngineClock(UINT16 EngineClock, UINT8 PerformanceLevel);
VOID Adl_SetMemoryClock(UINT16 MemoryClock, UINT8 PerformanceLevel);
VOID Adl_SetVoltage(UINT16 Voltage);
VOID Adl_SetMaxClocks();
VOID Adl_SetFanDutyCycle(int DutyCycle);

