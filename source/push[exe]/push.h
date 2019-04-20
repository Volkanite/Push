#ifndef PUSH_H
#define PUSH_H

#include <sl.h>
#include <pushbase.h>


///
/// Overlay name
///
#define PUSH_LIB_NAME_32 L"overlay32.dll"
#define PUSH_LIB_NAME_64 L"overlay64.dll"
///
/// Push version information
///
#define PUSH_VERSION        L"r50"

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
extern WCHAR PushFilePath[260];
extern BOOLEAN PushDriverLoaded;


VOID GetGamePath(
    WCHAR *pszGame,
    WCHAR *pszBuffer
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
VOID* BaseGetNamedObjectDirectory();


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

DWORD __stdcall MapFileAndCheckSumW(
    WCHAR* Filename,
    DWORD* HeaderSum,
    DWORD* CheckSum
    );

#define THREAD_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)
#define NtCurrentThread() ((HANDLE)(LONG)-2)

enum _THREAD_CREATE_FLAGS
{
    NoThreadFlags = 0x0000,
    CreateSuspended = 0x0001,
    NoDllCallbacks = 0x0002,
    HideFromDebug = 0x0004,
}THREAD_CREATE_FLAGS;

typedef char *  va_list;

typedef int (*TYPE_iswspace)(unsigned short C);
typedef int (*TYPE_memcmp)(const void *Buf1, const void *Buf2, int Size);
typedef void* (*TYPE_memcpy)(void *Dst, const void *Src, UINT32 Size);
typedef void (*TYPE_memset)(void *Region, DWORD Val, UINT32 Size);
typedef int(*TYPE_strcmp)(const char *Str1, const char *Str2);
typedef char* (*TYPE_strcpy)(char *Dest, const char *Source);
typedef UINT32 (*TYPE_strlen)(const char *Str);
typedef int (*TYPE_strncmp)(const char *Str1, const char *Str2, int MaxCount);
typedef char* (*TYPE_strncpy)(char *Dest, const char *Source, UINT32 Count);
typedef int (*TYPE_swscanf_s)(const wchar_t *Src, const wchar_t *Format, ...);
typedef int(*TYPE_vswprintf_s)(wchar_t *Dst, UINT32 SizeInWords, const wchar_t *Format, va_list ArgList);
typedef wchar_t* (*TYPE_wcsncat)(wchar_t *Dest, const wchar_t *Source, UINT32 Count);
typedef UINT32 (*TYPE_wcsnlen)(const wchar_t *Src, UINT32 MaxCount);
typedef __int32 (*TYPE_wcstol)(const wchar_t *Str, wchar_t **EndPtr, int Radix);
typedef int (*TYPE__wtoi)(const wchar_t *Str);


extern TYPE_iswspace    iswspace;
extern TYPE_memcmp      memcmp;
extern TYPE_memcpy      memcpy;
extern TYPE_memset      memset;
extern TYPE_strcmp      strcmp;
extern TYPE_strcpy      strcpy;
extern TYPE_strlen      strlen;
extern TYPE_strncmp     strncmp;
extern TYPE_strncpy     strncpy;
extern TYPE_swscanf_s   swscanf_s;
extern TYPE_vswprintf_s vswprintf_s;
extern TYPE_wcsncat     wcsncat;
extern TYPE_wcsnlen     wcsnlen;
extern TYPE_wcstol      wcstol;
extern TYPE__wtoi       _wtoi;

extern TYPE_memcmp      ntdll_memcmp;
extern TYPE_strcmp      ntdll_strcmp;

#include <hardware.h>
#include <gui.h>
#include <ring0.h>
#include <file.h>
#include <ramdisk.h>
#include <mij.h>
#include <driver.h>
#include <osd.h>
#include <mhp.h>

#endif //PUSH_H
