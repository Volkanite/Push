#ifndef PUSH_H
#define PUSH_H

#include <pushbase.h>


///
/// Library name
///
#define PUSH_LIB_NAME       "overlay.dll"

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





#ifdef __cplusplus
extern "C" VOID* PushHeapHandle;
//extern "C" VOID* PushIconHandle;
extern "C" VOID* PushInstance;
//extern "C" FILE_LIST PushFileList;
extern "C" UINT32 PushPageSize;
extern "C" PUSH_SHARED_MEMORY* PushSharedMemory;
extern "C" BOOLEAN g_ThreadListLock;
#else
extern VOID* PushHeapHandle;
//extern VOID* PushIconHandle;
extern VOID* PushInstance;
//extern FILE_LIST PushFileList;
extern UINT32 PushPageSize;
extern PUSH_SHARED_MEMORY* PushSharedMemory;
extern BOOLEAN g_ThreadListLock;
#endif

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
    
#ifdef __cplusplus
}
#endif //__cplusplus


#endif //PUSH_H
