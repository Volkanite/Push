class Process
{
public:
    static VOID Create( WCHAR* ImagePath, WCHAR* Args );
    static VOID* Open( PROCESSID processID, DWORD rights );
    static VOID Close( HANDLE ProcessHandle );
    static UINT32 GetId( WCHAR* ProcessName );
    static BOOLEAN ThreadExists( UINT32 ProcessId, UINT32 ThreadId );
    static NTSTATUS GetFileName( HANDLE ProcessHandle, WCHAR* FileName );
    static BOOLEAN GetFileName( PROCESSID processID, WCHAR* buffer );
    static VOID WriteMemory( HANDLE ProcessHandle, VOID* BaseAddress, VOID* Buffer, SIZE_T Size );
    static VOID Suspend( HANDLE ProcessHandle );
    static VOID Resume( HANDLE ProcessHandle );
    static BOOLEAN IsWow64( HANDLE ProcessHandle );
};
