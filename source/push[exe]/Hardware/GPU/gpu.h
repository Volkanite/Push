#ifndef GPU_H
#define GPU_H


typedef struct _GPU_ADAPTER
{
    UINT16(*GetEngineClock)();
    UINT16(*GetMemoryClock)();
    UINT64(*GetTotalMemory)();
    UINT64(*GetFreeMemory)();
    UINT8(*GetTemperature)();
    UINT8(*GetLoad)();
    UINT16(*GetMaximumEngineClock)();
    UINT16(*GetMaximumMemoryClock)();
    VOID(*ForceMaximumClocks)();
}GPU_ADAPTER;
#ifdef __cplusplus
extern "C" {
#endif
GPU_ADAPTER* CreateGpuAdapter( WORD VenderId );
#ifdef __cplusplus
}
#endif
#endif