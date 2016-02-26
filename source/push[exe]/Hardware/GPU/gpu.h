#ifndef GPU_H
#define GPU_H


typedef struct _GPU_ADAPTER
{
	WORD DeviceId;

    UINT16(*GetEngineClock)();
    UINT16(*GetMemoryClock)();
    UINT64(*GetTotalMemory)();
    UINT64(*GetFreeMemory)();
    UINT8(*GetTemperature)();
	UINT8(*GetLoad)();
    UINT16(*GetMaximumEngineClock)();
    UINT16(*GetMaximumMemoryClock)();
    UINT16(*GetVoltage)();
    UINT16(*GetFanSpeed)();
    VOID(*ForceMaximumClocks)();
}GPU_ADAPTER;


#ifdef __cplusplus
extern "C" {
#endif
	GPU_ADAPTER* CreateGpuAdapter(ULONG PciAddress);
#ifdef __cplusplus
}
#endif
#endif