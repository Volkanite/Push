#include "..\pushbase.h"

char *GetDirectoryFile(char *pszFileName);
//extern CHAR g_dir[260];
extern PUSH_SHARED_MEMORY *PushSharedMemory;
extern BOOLEAN g_DXGI;
extern UINT8    PushFrameBufferCount;
extern UINT32   PushFrameRate;
extern BOOLEAN PushStableFrameRate;
extern VOID PushRefreshThreadMonitor();
extern UINT8 PushGetMaxThreadUsage();
extern VOID PushOptimizeThreads();
extern UINT8    PushRefreshRate;
extern UINT8    PushAcceptableFps;

