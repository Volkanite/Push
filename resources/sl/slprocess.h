typedef VOID(*ENUM_PROCESSES_CALLBACK)(
    SYSTEM_PROCESS_INFORMATION* ProcessInformation
    );

VOID Process_Create(WCHAR* ImagePath, WCHAR* Args, WCHAR* WorkingDirectory);
VOID* Process_Open(UINT32 processID, DWORD rights);
VOID Process_Close(HANDLE ProcessHandle);
UINT32 Process_GetId(WCHAR* ProcessName, DWORD Ignore);
BOOLEAN Process_ThreadExists(UINT32 ProcessId, UINT32 ThreadId);
NTSTATUS Process_GetFileNameByHandle(HANDLE ProcessHandle, WCHAR* FileName);
BOOLEAN Process_GetFileNameByProcessId(UINT32 processID, WCHAR* buffer);
VOID Process_WriteMemory(HANDLE ProcessHandle, VOID* BaseAddress, VOID* Buffer, SIZE_T Size);
VOID Process_Suspend(HANDLE ProcessHandle);
VOID Process_Resume(HANDLE ProcessHandle);
VOID Process_Terminate(HANDLE ProcessHandle);
BOOLEAN Process_IsWow64(HANDLE ProcessHandle);
VOID Process_EnumProcesses( ENUM_PROCESSES_CALLBACK Callback );