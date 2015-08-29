#include "slnt.h"

#ifdef __cplusplus
extern "C" {
#endif

VOID* __stdcall OpenSCManagerW(
    WCHAR*  lpMachineName,
    WCHAR*  lpDatabaseName,
    DWORD   dwDesiredAccess
    );

VOID*
__stdcall
OpenServiceW(
    VOID*   hSCManager,
    WCHAR*  lpServiceName,
    DWORD   dwDesiredAccess
    );

VOID*
__stdcall
CreateServiceW(
    VOID*   hSCManager,
    WCHAR*  lpServiceName,
    WCHAR*  lpDisplayName,
    DWORD   dwDesiredAccess,
    DWORD   dwServiceType,
    DWORD   dwStartType,
    DWORD   dwErrorControl,
    WCHAR*  lpBinaryPathName,
    WCHAR*  lpLoadOrderGroup,
    DWORD*  lpdwTagId,
    WCHAR*  lpDependencies,
    WCHAR*  lpServiceStartName,
    WCHAR*  lpPassword
    );

INT32
__stdcall
QueryServiceConfigW(
    VOID*                   hService,
    QUERY_SERVICE_CONFIG*   lpServiceConfig,
    DWORD                   cbBufSize,
    DWORD*                  pcbBytesNeeded
    );

INT32
__stdcall
ChangeServiceConfigW(
    VOID*   hService,
    DWORD   dwServiceType,
    DWORD   dwStartType,
    DWORD   dwErrorControl,
    WCHAR*  lpBinaryPathName,
    WCHAR*  lpLoadOrderGroup,
    DWORD   lpdwTagId,
    WCHAR*  lpDependencies,
    WCHAR*  lpServiceStartName,
    WCHAR*  lpPassword,
    WCHAR*  lpDisplayName
    );

INT32
__stdcall
StartServiceW(
    VOID*   hService,
    DWORD   dwNumServiceArgs,
    WCHAR*  lpServiceArgVectors
    );

INT32
__stdcall
QueryServiceStatus(
    VOID*           hService,
    SERVICE_STATUS* lpServiceStatus
    );

INT32
__stdcall
CloseServiceHandle( VOID* hSCObject );

BYTE __stdcall FlushFileBuffers(
    VOID *hFile
    );

VOID __stdcall InitCommonControls(
    VOID
    );

VOID*
__stdcall
CreateWindowExW(
    DWORD   dwExStyle,
    WCHAR*  lpClassName,
    WCHAR*  lpWindowName,
    DWORD   dwStyle,
    INT32   x,
    INT32   y,
    INT32   nWidth,
    INT32   nHeight,
    VOID*   hWndParent,
    VOID*   hMenu,
    VOID*   hInstance,
    VOID*   lpParam
    );

int
__stdcall
DestroyWindow( VOID *hWnd );

int
__stdcall
IsWindow( VOID *hWnd );

int
__stdcall
ShowWindow(
    VOID    *hWnd,
    int     nCmdShow
    );

int
__stdcall
RedrawWindow(
    VOID*   hWnd,
    RECT*   lprcUpdate,
    VOID*   hrgnUpdate,
    UINT32  flags
    );

VOID*
__stdcall
SetParent(
    VOID *hWndChild,
    VOID *hWndNewParent
    );

VOID*
__stdcall
SetActiveWindow( VOID *hWnd );

INT32
__stdcall
SetForegroundWindow( VOID *hWnd );

LONG
__stdcall
SetWindowLongW(
    VOID*   hWnd,
    INT32   nIndex,
    LONG    dwNewLong
    );

LONG
__stdcall
GetWindowLongW(
    VOID* hWnd,
    INT32 nIndex
    );

INT32
__stdcall
GetWindowTextW(
    VOID*   hWnd,
    WCHAR*  lpString,
    INT32   nMaxCount);

UINT32
__stdcall
SetTimer(
    VOID*   hWnd,
    UINT32  nIDEvent,
    UINT32  uElapse,
    FARPROC lpTimerFunc
    );

UINT32
__stdcall
RegisterWindowMessageW( WCHAR* lpString );

LONG
__stdcall
DefWindowProcW(
    VOID*   hWnd,
    UINT32  Msg,
    UINT32  wParam,
    LONG    lParam
    );

INT32
__stdcall
GetMessageW(
    MSG*    lpMsg,
    VOID*   hWnd,
    UINT32  wMsgFilterMin,
    UINT32  wMsgFilterMax
    );

INT32
__stdcall
TranslateMessage( MSG *lpMsg );

LONG
__stdcall
DispatchMessageW( MSG *lpMsg );

INT32
__stdcall
PostMessageW(
    VOID*   hWnd,
    UINT32  Msg,
    UINT32  wParam,
    LONG    lParam
    );

VOID
__stdcall
PostQuitMessage( INT32 nExitCode );

VOID*
__stdcall
LoadMenuW(
    VOID*   hInstance,
    WCHAR*  lpMenuName
    );

INT32
__stdcall
DestroyMenu( VOID *hMenu );

VOID*
__stdcall
GetSubMenu(
    VOID* hMenu,
    INT32 nPos
    );

INT32
__stdcall
SetMenuDefaultItem(
    VOID*   hMenu,
    UINT32  uItem,
    UINT32  fByPos
    );

unsigned int
__stdcall
GetMenuItemID(
    VOID    *hMenu,
    int     nPos
    );

int
__stdcall
TrackPopupMenu(
    VOID            *hMenu,
    unsigned int    uFlags,
    int             x,
    int             y,
    int             nReserved,
    VOID            *hWnd,
    const RECT      *prcRect
    );

int
__stdcall
GetClientRect(
    VOID *hWnd,
    RECT *lpRect
    );

INT32
__stdcall
GetWindowRect(
    VOID *hWnd,
    RECT *lpRect
);

INT32
__stdcall
GetCursorPos( POINT *lpPoint );

INT32
__stdcall
SetWindowPos(
    VOID    *hWnd,
    VOID    *hWndInsertAfter,
    INT32   X,
    INT32   Y,
    INT32   cx,
    INT32   cy,
    UINT32  uFlags
);

INT32
__stdcall
MessageBoxW(
    VOID*   hWnd,
    WCHAR*  lpText,
    WCHAR*  lpCaption,
    UINT32  uType
    );

INT32
__stdcall
LoadStringW(
    VOID*   hInstance,
    UINT32  uID,
    WCHAR*  lpBuffer,
    INT32   cchBufferMax
    );

VOID*
__stdcall
LoadIconW(
    VOID*   hInstance,
    WCHAR*  lpIconName
    );

DWORD
__stdcall
GetCurrentDirectoryW(
    DWORD   nBufferLength,
    WCHAR*  lpBuffer
    );

LONG
__stdcall
SendMessageW(
    VOID*   hWnd,
    UINT32  Msg,
    UINT32  wParam,
    LONG    lParam
    );

INT32
__stdcall
RemoveDirectoryW( WCHAR* lpPathName );

INT32
__stdcall
DeleteFileW( WCHAR* lpFileName );

int __stdcall FindNextFileW(
    VOID*               hFindFile,
    WIN32_FIND_DATA*    lpFindFileData
    );

int
__stdcall
FindClose( VOID *hFindFile );

BYTE
__stdcall
CreateSymbolicLinkW(
    WCHAR*  lpSymlinkFileName,
    WCHAR*  lpTargetFileName,
    DWORD   dwFlags
    );

DWORD
__stdcall
GetFileAttributesW(
  WCHAR* lpFileName
);

 INT32 __stdcall DefineDosDeviceW(
    DWORD       dwFlags,
    WCHAR   *lpDeviceName,
    WCHAR   *lpTargetPath
    );

DWORD
__stdcall
QueryDosDeviceW(
    WCHAR*  lpDeviceName,
    WCHAR*  lpTargetPath,
    DWORD   ucchMax
    );

DWORD
__stdcall
GetLogicalDrives( VOID );

VOID*
__stdcall
GetModuleHandleW( WCHAR* lpModuleName );

DWORD
__stdcall
GetModuleFileNameW(
    VOID*   hModule,
    WCHAR*  lpFilename,
    DWORD   nSize
    );

VOID*
__stdcall
VirtualAllocEx(
    VOID    *hProcess,
    VOID    *lpAddress,
    DWORD   dwSize,
    DWORD   flAllocationType,
    DWORD   flProtect
    );

SDWORD
__stdcall
VirtualFreeEx(
    VOID    *hProcess,
    VOID    *lpAddress,
    DWORD   dwSize,
    DWORD   dwFreeType
    );

 INT32 __stdcall NtOpenProcess(
     VOID **ProcessHandle,
     DWORD AccessMask,
     OBJECT_ATTRIBUTES *ObjectAttributes,
     CLIENT_ID *ClientId );

INT32
__stdcall
WriteProcessMemory(
    VOID*   hProcess,
    VOID*   lpBaseAddress,
    VOID*   lpBuffer,
    DWORD   nSize,
    DWORD*  lpNumberOfBytesWritten
    );

VOID*
__stdcall
CreateRemoteThread(
    VOID                    *hProcess,
    SECURITY_ATTRIBUTES     *lpThreadAttributes,
    DWORD                   dwStackSize,
    PTHREAD_START_ROUTINE   lpStartAddress,
    VOID                    *lpParameter,
    DWORD                   dwCreationFlags,
    DWORD                   *lpThreadId
    );

INT32
__stdcall ControlService(
  VOID *hService,
  DWORD dwControl,
  SERVICE_STATUS *lpServiceStatus
);


VOID*
__stdcall
GetCurrentThread( VOID );

DWORD
__stdcall
SetThreadAffinityMask(
    VOID    *hThread,
    DWORD   dwThreadAffinityMask
    );

int __stdcall GetExitCodeThread(
    VOID    *hThread,
    DWORD   *lpExitCode
    );

INTBOOL __stdcall GetSystemTimes(
    FILETIME *lpIdleTime,
    FILETIME *lpKernelTime,
    FILETIME *lpUserTime
    );

VOID
__stdcall
Sleep(
  DWORD dwMilliseconds
);


VOID*
__stdcall
CreateEventW(
    SECURITY_ATTRIBUTES *lpEventAttributes,
    BYTE                bManualReset,
    BYTE                bInitialState,
    const WCHAR         *lpName
    );

VOID*
__stdcall
OpenEventW(
    DWORD       dwDesiredAccess,
    BYTE        bInheritHandle,
    const WCHAR *lpName
    );

SDWORD
__stdcall
SetEvent( VOID *hEvent );

SDWORD
__stdcall
ResetEvent( VOID *hEvent );

DWORD
__stdcall
WaitForSingleObject(
    VOID    *hHandle,
    DWORD   dwMilliseconds
    );

DWORD __stdcall WaitForMultipleObjectsEx(
    DWORD nCount,
    VOID** lpHandles,
    INTBOOL bWaitAll,
    DWORD dwMilliseconds,
    INTBOOL bAlertable
    );

DWORD
__stdcall
GetTickCount( VOID );

VOID*
__stdcall
CreateMutexW(
    SECURITY_ATTRIBUTES*    lpMutexAttributes,
    BYTE                    bInitialOwner,
    WCHAR*                  lpName
    );

VOID
__stdcall
SetLastError( DWORD dwErrCode );

DWORD
__stdcall
GetLastError( VOID );

__declspec(noreturn)
VOID
__stdcall
ExitProcess( UINT32 uExitCode );

INT32
__stdcall
CallNamedPipeW(
    WCHAR*  lpNamedPipeName,
    VOID*   lpInBuffer,
    UINT32  nInBufferSize,
    VOID*   lpOutBuffer,
    UINT32  nOutBufferSize,
    UINT32* lpBytesRead,
    UINT32  nTimeOut
    );

INT32
__stdcall
GetOverlappedResult(
    VOID        *hFile,
    OVERLAPPED  *lpOverlapped,
    DWORD       *lpNumberOfBytesTransferred,
    INT32       bWait
    );

INT32
__stdcall
WideCharToMultiByte(
    UINT32          CodePage,
    DWORD           dwFlags,
    const WCHAR     *lpWideCharStr,
    INT32           cchWideChar,
    CHAR            *lpMultiByteStr,
    INT32           cbMultiByte,
    const CHAR      *lpDefaultChar,
    INT32           *lpUsedDefaultChar);

BYTE
__stdcall
RtlCreateUnicodeString(
    UNICODE_STRING  *DestinationString,
    const WCHAR     *SourceString
    );

VOID
__stdcall
RtlFreeUnicodeString( UNICODE_STRING *UnicodeString );



typedef
VOID
(__stdcall *PIO_APC_ROUTINE) (
    VOID *ApcContext,
    IO_STATUS_BLOCK *IoStatusBlock,
    UINT32 Reserved
    );
 INT32 __stdcall NtDeviceIoControlFile(
    VOID *FileHandle,
    VOID *Event,
    PIO_APC_ROUTINE ApcRoutine,
    VOID *ApcContext,
    IO_STATUS_BLOCK *IoStatusBlock,
    UINT32 IoControlCode,
    VOID *InputBuffer,
    UINT32 InputBufferLength,
    VOID *OutputBuffer,
    UINT32 OutputBufferLength
);

DWORD
__stdcall
RtlNtStatusToDosError( long Status );


INT32 __stdcall AllocConsole(VOID);



INT32 __stdcall FreeLibrary(
  VOID *hModule
  );

LONG __stdcall NtQueryPerformanceCounter(
  LARGE_INTEGER* PerformanceCounter,
  LARGE_INTEGER* PerformanceFrequency
  );

INT32 __stdcall VirtualProtect(
  VOID* lpAddress,
  UINT_B dwSize,
  DWORD flNewProtect,
  DWORD* lpflOldProtect
);

INT32 __stdcall FlushInstructionCache(
  VOID* hProcess,
  VOID* lpBaseAddress,
  UINT_B dwSize
);

DWORD __stdcall GetSecurityInfo(
    VOID*           handle,
    SE_OBJECT_TYPE  ObjectType,
    DWORD           SecurityInfo,
    VOID**          ppsidOwner,
    VOID**          ppsidGroup,
    ACL**           ppDacl,
    ACL**           ppSacl,
    VOID**          ppSecurityDescriptor
    );

DWORD __stdcall SetSecurityInfo(
    VOID*                handle,
    SE_OBJECT_TYPE        ObjectType,
    DWORD  SecurityInfo,
    VOID*                  psidOwner,
    VOID*                  psidGroup,
    ACL*    pDacl,
    ACL*                  pSacl
    );

VOID* __stdcall LocalFree(
    VOID* hMem
    );

INT32 __stdcall MultiByteToWideChar(
    UINT32  CodePage,
    DWORD   dwFlags,
    CHAR*   lpMultiByteStr,
    INT32   cbMultiByte,
    WCHAR*  lpWideCharStr,
    INT32   cchWideChar
    );

WORD __stdcall RegisterClassExW(
    WNDCLASSEX *lpwcx
    );

INTBOOL __stdcall SetPropW(
    VOID*   hWnd,
    WCHAR*  lpString,
    VOID*   hData
    );

// Nt

#if defined(MSVC)
TEB* __stdcall NtCurrentTeb(
    );
#else
static __inline__ struct _TEB * NtCurrentTeb(void)
{
    struct _TEB *ret;

    __asm__ __volatile__ (
        "mov{l} {%%fs:0x18,%0|%0,%%fs:0x18}\n"
        : "=r" (ret)
        : /* no inputs */
    );

    return ret;
}
#endif

INT32 __stdcall NtOpenProcessToken(
    VOID *ProcessHandle,
    DWORD DesiredAccess,
    VOID **TokenHandle
    );

INT32 __stdcall NtAdjustPrivilegesToken(
    VOID *TokenHandle,
    BOOLEAN DisableAllPrivileges,
    TOKEN_PRIVILEGES *TokenPrivileges,
    DWORD PreviousPrivilegesLength,
    TOKEN_PRIVILEGES *PreviousPrivileges,
    DWORD *RequiredLength
    );

 INT32 __stdcall NtQueryInformationThread(
    VOID *ThreadHandle,
    UINT32 ThreadInformationClass,
    VOID *ThreadInformation,
    UINT32 ThreadInformationLength,
    UINT32 *ReturnLength );

 INT32 __stdcall NtAllocateVirtualMemory(
    VOID *ProcessHandle,
    VOID **BaseAddress,
    UINT_B ZeroBits,
    UINT_B *RegionSize,
    UINT32 AllocationType,
    UINT32 Protect );

 INT32 __stdcall NtFreeVirtualMemory(
    VOID *ProcessHandle,
    VOID **BaseAddress,
    UINT32 *RegionSize,
    UINT32 FreeType );

INT32 __stdcall NtLoadDriver(
    UNICODE_STRING *pDrvName
    );

INT32 __stdcall NtOpenThread(
    VOID **ThreadHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES *ObjectAttributes,
    CLIENT_ID *ClientId
    );

INT32 __stdcall NtWaitForSingleObject(
    VOID *Handle,
    BOOLEAN Alertable,
    LARGE_INTEGER *Timeout
    );

LONG __stdcall NtQueryDirectoryFile(
    VOID*                   FileHandle,
    VOID*                   Event,
    PIO_APC_ROUTINE         ApcRoutine,
    VOID*                   ApcContext,
    IO_STATUS_BLOCK*        IoStatusBlock,
    VOID*                   FileInformation,
    ULONG                   Length,
    FILE_INFORMATION_CLASS  FileInformationClass,
    BOOLEAN                    ReturnSingleEntry,
    UNICODE_STRING*         FileName,
    BOOLEAN                    RestartScan
    );

LONG __stdcall NtCreateFile(
    VOID**              FileHandle,
    DWORD               DesiredAccess,
    OBJECT_ATTRIBUTES*  ObjectAttributes,
    IO_STATUS_BLOCK*    IoStatusBlock,
    LARGE_INTEGER*      AllocationSize,
    ULONG               FileAttributes,
    ULONG               ShareAccess,
    ULONG               CreateDisposition,
    ULONG               CreateOptions,
    VOID*               EaBuffer,
    ULONG               EaLength
    );

LONG __stdcall NtReadFile(
    VOID*               FileHandle,
    VOID*               Event,
    PIO_APC_ROUTINE     ApcRoutine,
    VOID*               ApcContext,
    IO_STATUS_BLOCK*    IoStatusBlock,
    VOID*               Buffer,
    ULONG               Length,
    LARGE_INTEGER*      ByteOffset,
    ULONG*              Key
    );

LONG __stdcall NtSetInformationFile(
    VOID*                   FileHandle,
    IO_STATUS_BLOCK*        IoStatusBlock,
    VOID*                   FileInformation,
    ULONG                   Length,
    FILE_INFORMATION_CLASS  FileInformationClass
    );

NTSTATUS
__stdcall
NtQueryInformationFile(
    VOID* FileHandle,
    IO_STATUS_BLOCK* IoStatusBlock,
    VOID* FileInformation,
    ULONG Length,
    FILE_INFORMATION_CLASS FileInformationClass
    );

// Rtl

LONG __stdcall RtlInitializeCriticalSection(
    RTL_CRITICAL_SECTION* CriticalSection
    );

VOID* __stdcall RtlAllocateHeap(
    VOID*           HeapHandle,
    DWORD           Flags,
    UINT_B    Size
    );

BOOLEAN __stdcall RtlFreeHeap(
    VOID* HeapHandle,
    DWORD Flags,
    VOID* HeapBase
    );


VOID* __stdcall RtlReAllocateHeap(
    VOID*   HeapHandle,
    DWORD   Flags,
    VOID*   MemoryPointer,
    UINT32  Size
    );

LONG __stdcall RtlCompareUnicodeString(
    UNICODE_STRING *String1,
    UNICODE_STRING *String2,
    BOOLEAN CaseInSensitive
    );

BOOLEAN __stdcall RtlDosPathNameToNtPathName_U(
    WCHAR*                  DosFileName,
    UNICODE_STRING*         NtFileName,
    WCHAR**                 FilePart,
    RTL_RELATIVE_NAME_U*    RelativeName
    );

NTSTATUS __stdcall NtCreateSection(
    VOID** SectionHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes,
    LARGE_INTEGER* MaximumSize,
    ULONG SectionPageProtection,
    ULONG AllocationAttributes,
    VOID* FileHandle
    );


void __stdcall OutputDebugStringW(
    WCHAR* lpOutputString
    );

    LONG __stdcall RtlEnterCriticalSection(
    RTL_CRITICAL_SECTION* CriticalSection
    );

LONG __stdcall LdrLoadDll(
    WCHAR* SearchPath,
    DWORD* LoadFlags,
    UNICODE_STRING* Name,
    VOID** BaseAddress
    );

#ifdef __cplusplus
}
#endif




