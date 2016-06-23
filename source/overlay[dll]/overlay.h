#include "..\pushbase.h"

extern PUSH_SHARED_MEMORY *PushSharedMemory;
extern BOOLEAN g_DXGI;
extern UINT8    PushRefreshRate;
extern UINT8 AcceptableFps;
extern UINT16 DiskResponseTime;
extern UINT32 FrameRate;
extern BOOLEAN IsStableFramerate;

char *GetDirectoryFile(char *pszFileName);
VOID CallPipe(WCHAR* Command, UINT16* Output);
extern VOID PushRefreshThreadMonitor();
extern UINT8 PushGetMaxThreadUsage();
extern VOID PushOptimizeThreads();
