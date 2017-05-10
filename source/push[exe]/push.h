#ifndef PUSH_H
#define PUSH_H

#include <pushbase.h>


///
/// Overlay name
///
#define PUSH_LIB_NAME_32 L"overlay32.dll"
#define PUSH_LIB_NAME_64 L"overlay64.dll"

///
/// Settings file name
///
//#define PUSH_SETTINGS_FILE    "push.ini"

///
/// FileReader
///
#define FILE_READER_BUFFER_LEN 500


#define WM_ICON_NOTIFY WM_APP+10


typedef struct _FILE_LIST_ENTRY FILE_LIST_ENTRY;
typedef struct _FILE_LIST_ENTRY
{
    WCHAR *Name;
    UINT32 Bytes;
    BOOLEAN Cache;
    FILE_LIST_ENTRY* NextEntry;

} FILE_LIST_ENTRY, *FILE_LIST;

typedef enum _OVERLAY_INTERFACE
{
    OVERLAY_INTERFACE_PURE,
    OVERLAY_INTERFACE_RTSS,

} OVERLAY_INTERFACE;


extern VOID* PushHeapHandle;
extern VOID* PushInstance;
extern PUSH_SHARED_MEMORY* PushSharedMemory;
extern BOOLEAN g_ThreadListLock;
extern OVERLAY_INTERFACE PushOverlayInterface;
extern BOOLEAN             g_bRecache;
extern UINT32 GameProcessId;


VOID GetGamePath(
    WCHAR *pszGame,
    WCHAR *pszBuffer
    );

VOID GetTime(
    CHAR *pszBuffer
    );

VOID VerifyLib(
    WCHAR *libName
    );

VOID PushBuildCacheList(
    WCHAR* Directory,
    FILE_DIRECTORY_INFORMATION* Information
    );

VOID PushAddToFileList(
    FILE_LIST* FileList,
    FILE_LIST_ENTRY *FileEntry
    );

VOID PushOnTimer();

VOID
BuildGameFilesList(
    WCHAR* Directory,
    FILE_DIRECTORY_INFORMATION* Information
    );
#ifdef __cplusplus
extern "C" {
#endif
BOOLEAN ExtractResource(
    WCHAR* ResourceName,
    WCHAR* OutputPath
    );
#ifdef __cplusplus
}
#endif

VOID PushToggleProcessMonitoring(
    BOOLEAN Activate
    );
    
VOID PushGetProcessInfo(
    PROCESS_CALLBACK_INFO* ProcessInformation
    );

VOID PushGetThreadInfo(
    THREAD_CALLBACK_INFO* ThreadInformation
    );

VOID PushGetImageInfo(
    IMAGE_CALLBACK_INFO* ImageInformation
    );

VOID Log(const wchar_t* Format, ...);
VOID Push_FormatTime(WCHAR* Buffer);
VOID* PushBaseGetNamedObjectDirectory();

#define OBJ_OPENIF   0x00000080L

typedef enum _SECTION_INHERIT
{
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;
typedef struct _PS_ATTRIBUTE
{
    ULONG_PTR Attribute;
    SIZE_T Size;
    union
    {
        ULONG_PTR Value;
        VOID* ValuePtr;
    };
    SIZE_T* ReturnLength;
} PS_ATTRIBUTE, *PPS_ATTRIBUTE;
typedef struct _PS_ATTRIBUTE_LIST
{
    SIZE_T TotalLength;
    PS_ATTRIBUTE Attributes[1];
} PS_ATTRIBUTE_LIST, *PPS_ATTRIBUTE_LIST;


NTSTATUS __stdcall NtMapViewOfSection(
    VOID* SectionHandle,
    VOID* ProcessHandle,
    VOID** BaseAddress,
    UINT_B ZeroBits,
    UINT_B CommitSize,
    LARGE_INTEGER* SectionOffset,
    SIZE_T* ViewSize,
    SECTION_INHERIT InheritDisposition,
    ULONG AllocationType,
    ULONG Win32Protect
    );

NTSTATUS __stdcall NtCreateThreadEx(
    HANDLE* ThreadHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes,
    HANDLE ProcessHandle,
    VOID* StartRoutine, // PUSER_THREAD_START_ROUTINE
    VOID* Argument,
    ULONG CreateFlags, // THREAD_CREATE_FLAGS_*
    SIZE_T ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PPS_ATTRIBUTE_LIST AttributeList
    );

#define THREAD_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)

enum _THREAD_CREATE_FLAGS
{
    NoThreadFlags = 0x0000,
    CreateSuspended = 0x0001,
    NoDllCallbacks = 0x0002,
    HideFromDebug = 0x0004,
}THREAD_CREATE_FLAGS;


#endif //PUSH_H
