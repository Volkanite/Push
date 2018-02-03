#include "..\pushbase.h"

extern PUSH_SHARED_MEMORY *PushSharedMemory;
extern BOOLEAN g_DXGI;
extern UINT16 DiskResponseTime;
extern double FrameRate;
extern BOOLEAN IsStableFramerate;

char *GetDirectoryFile(char *pszFileName);
VOID Log(const wchar_t* Format, ...);
VOID CallPipe(WCHAR* Command, UINT16* Output);
extern VOID PushRefreshThreadMonitor();
extern UINT8 PushGetMaxThreadUsage();
extern VOID PushOptimizeThreads();
