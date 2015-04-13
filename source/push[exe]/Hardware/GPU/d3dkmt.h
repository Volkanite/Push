#ifdef __cplusplus
extern "C" {
#endif

VOID D3DKMTInitialize();
UINT8 D3DKMTGetGpuUsage();
VOID D3DKMT_GetPrivateDriverData(VOID* PrivateDriverData, UINT32 PrivateDriverDataSize);

#ifdef __cplusplus
}
#endif