#include "..\pushbase.h"

extern PUSH_SHARED_MEMORY *PushSharedMemory;
extern BOOLEAN g_DXGI;
extern VOID PushRefreshThreadMonitor();
extern UINT8 PushGetMaxThreadUsage();
extern VOID PushOptimizeThreads();
extern UINT8    PushRefreshRate;
extern UINT8    PushAcceptableFps;
extern UINT16 DiskResponseTime;
extern UINT32 FrameRate;

char *GetDirectoryFile(char *pszFileName);
VOID CallPipe(WCHAR* Command, UINT16* Output);

