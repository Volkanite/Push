// Define CopyFileEx option flags

#define COPY_FILE_FAIL_IF_EXISTS 0x00000001
#define DRIVE_REMOTE      4

typedef
DWORD
(__stdcall *LPPROGRESS_ROUTINE)(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    VOID* hSourceFile,
    VOID* hDestinationFile,
    VOID* lpData
    );

UINT32 __stdcall GetDriveTypeW(
    WCHAR* lpRootPathName
    );

DWORD __stdcall GetTempPathW(
    DWORD nBufferLength,
    WCHAR* lpBuffer
    );

DWORD __stdcall SizeofResource(
    HANDLE hModule,
    HANDLE hResInfo
    );

VOID* __stdcall LockResource(
    HANDLE hResData
    );

HANDLE __stdcall LoadResource(
    HANDLE hModule,
    HANDLE hResInfo
    );

HANDLE __stdcall FindResourceW(
    HANDLE hModule,
    WCHAR* lpName,
    WCHAR* lpType
    );

INTBOOL __stdcall IsWow64Process(
    HANDLE hProcess,
    INTBOOL* Wow64Process
    );
