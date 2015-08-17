#include "..\pushbase.h"

char *GetDirectoryFile(char *pszFileName);
extern PUSH_SHARED_MEMORY *PushSharedMemory;
extern BOOLEAN g_DXGI;
extern VOID PushRefreshThreadMonitor();
extern UINT8 PushGetMaxThreadUsage();
extern VOID PushOptimizeThreads();
extern UINT8    PushRefreshRate;
extern UINT8    PushAcceptableFps;
extern UINT16 DiskResponseTime;
VOID CallPipe(WCHAR* Command, UINT16* Output);

