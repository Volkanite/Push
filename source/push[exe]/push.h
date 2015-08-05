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


#endif //PUSH_H
