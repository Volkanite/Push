#ifdef __cplusplus
extern "C" {
#endif

VOID D3DKMTInitialize();
UINT8 D3DKMT_GetGpuUsage();
UINT16 D3DKMT_GetEngineClock();
UINT16 D3DKMT_GetMemoryClock();
UINT16 D3DKMT_GetMaxEngineClock();
UINT16 D3DKMT_GetMaxMemoryClock();
UINT64 D3DKMT_GetTotalMemory();
UINT64 D3DKMT_GetFreeMemory();
UINT8  D3DKMT_GetTemperature();
VOID D3DKMT_ForceMaximumClocks();
VOID D3DKMT_GetPrivateDriverData(VOID* PrivateDriverData, UINT32 PrivateDriverDataSize);

#ifdef __cplusplus
}
#endif