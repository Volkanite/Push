#ifndef _WIN_H
#define _WIN_H

#pragma once
#include "ntbasic.h"
#define INVALID_HANDLE_VALUE    ((VOID *)(SDWORD)-1)
#define CW_USEDEFAULT           ((SDWORD)0x80000000)
#define MAKEINTRESOURCE(i)      ((WCHAR*)((DWORD)((WORD)(i))))
#define SPI_SETWORKAREA         0x002F
#define HIWORD(l)               ((WORD)((((DWORD)(l)) >> 16) & 0xffff))
#define LOWORD(l)               ((WORD)(((DWORD)(l)) & 0xffff))
#define GWL_STYLE               (-16)
#define INFINITE                0xFFFFFFFF
#define _HRESULT_TYPEDEF_(_sc)  ((SDWORD)_sc)
#define E_FAIL                           _HRESULT_TYPEDEF_(0x80004005L)
#define S_OK                    ((SDWORD)0L)
#define STATUS_WAIT_0                    ((DWORD)0x00000000L)
#define WAIT_OBJECT_0           ((STATUS_WAIT_0 ) + 0 )
#define MAKELONG(a, b)      ((SDWORD)(((WORD)(((DWORD)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD)(b)) & 0xffff))) << 16))
#define MAKELPARAM(l, h)      ((SDWORD)(DWORD)MAKELONG(l, h))
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define             FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define NMPWAIT_WAIT_FOREVER            0xffffffff
#define NtCurrentProcess() ((VOID*)(UINT_B)-1)

#define ERROR_ACCESS_DENIED     5L
#define ERROR_ALREADY_EXISTS    183L

#define FILE_BEGIN 0
#define FILE_SHARE_READ     0x00000001
#define FILE_SHARE_WRITE    0x00000002


#define FILE_ATTRIBUTE_DIRECTORY    0x00000010
#define FILE_ATTRIBUTE_NORMAL       0x00000080

#define SYNCHRONIZE                      (0x00100000L)
#define DELETE                           (0x00010000L)

#define HEAP_NO_SERIALIZE               0x00000001
#define HEAP_ZERO_MEMORY                0x00000008


#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)


//
//  These are the generic rights.
//

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)

#define PAGE_READWRITE  0x04
#define MEM_COMMIT      0x1000
#define MEM_RELEASE     0x8000

#define SECTION_QUERY                0x0001
#define SECTION_MAP_WRITE            0x0002
#define SECTION_MAP_READ             0x0004
#define SECTION_MAP_EXECUTE          0x0008
#define SECTION_EXTEND_SIZE          0x0010
#define SECTION_MAP_EXECUTE_EXPLICIT 0x0020 // not included in SECTION_ALL_ACCESS

#define SECTION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SECTION_QUERY|\
                            SECTION_MAP_WRITE |      \
                            SECTION_MAP_READ |       \
                            SECTION_MAP_EXECUTE |    \
                            SECTION_EXTEND_SIZE)

#define FILE_MAP_ALL_ACCESS SECTION_ALL_ACCESS

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define FSCTL_LOCK_VOLUME               CTL_CODE(0x00000009,  6, 0, 0)
#define FSCTL_UNLOCK_VOLUME             CTL_CODE(0x00000009,  7, 0, 0)
#define FSCTL_DISMOUNT_VOLUME           CTL_CODE(0x00000009,  8, 0, 0)
#define FSCTL_GET_REPARSE_POINT         CTL_CODE(0x00000009, 42, 0, 0)

#define IOCTL_STORAGE_EJECT_MEDIA       CTL_CODE(0x0000002d, 0x0202, 0, ( 0x0001 ))


#define DDD_RAW_TARGET_PATH         0x00000001
#define DDD_REMOVE_DEFINITION       0x00000002


#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#define CS_DBLCLKS 0x0008


#define WS_OVERLAPPED       0x00000000L
#define WS_POPUP            0x80000000L
#define WS_CHILD            0x40000000L
#define WS_VISIBLE          0x10000000L
#define WS_CAPTION          0x00C00000L
#define WS_BORDER           0x00800000L
#define WS_SYSMENU          0x00080000L
#define WS_THICKFRAME       0x00040000L

#define WS_EX_CLIENTEDGE    0x00000200L

#define RDW_INVALIDATE  0x0001
#define RDW_ERASE       0x0004
#define RDW_ALLCHILDREN 0x0080
#define RDW_UPDATENOW   0x0100
#define RDW_FRAME       0x0400


#define NIF_MESSAGE 0x00000001
#define NIF_ICON    0x00000002
#define NIF_TIP     0x00000004

#define NIM_ADD     0x00000000
#define NIM_MODIFY  0x00000001
#define NIM_DELETE  0x00000002


#define LVM_FIRST                       0x1000

#define WC_LISTVIEW                     "SysListView32"

#define LVS_REPORT                      0x0001
#define LVS_EDITLABELS                  0x0200

#define LVCF_FMT                        0x0001
#define LVCF_WIDTH                      0x0002
#define LVCF_TEXT                       0x0004
#define LVCF_SUBITEM                    0x0008

#define LVCFMT_LEFT                     0x0000

/* ListView Messages */

#define LVM_GETITEMCOUNT                (LVM_FIRST + 4)
#define LVM_DELETEITEM                  (LVM_FIRST + 8)
#define LVM_DELETEALLITEMS              (LVM_FIRST + 9)
#define LVM_GETNEXTITEM                 (LVM_FIRST + 12)
#define LVM_GETITEMRECT                 (LVM_FIRST + 14)
#define LVM_SETITEMPOSITION             (LVM_FIRST + 15)
#define LVM_GETITEMPOSITION             (LVM_FIRST + 16)
#define LVM_HITTEST                     (LVM_FIRST + 18)
#define LVM_ENSUREVISIBLE               (LVM_FIRST + 19)
#define LVM_SCROLL                      (LVM_FIRST + 20)
#define LVM_REDRAWITEMS                 (LVM_FIRST + 21)
#define LVM_ARRANGE                     (LVM_FIRST + 22)
#define LVM_GETCOLUMNWIDTH              (LVM_FIRST + 29)
#define LVM_SETCOLUMNWIDTH              (LVM_FIRST + 30)
#define LVM_SETITEMSTATE                (LVM_FIRST + 43)
#define LVM_SORTITEMS                   (LVM_FIRST + 48)
#define LVM_SETEXTENDEDLISTVIEWSTYLE    (LVM_FIRST + 54)
#define LVM_GETITEM                     (LVM_FIRST + 75)
#define LVM_SETITEM                     (LVM_FIRST + 76)
#define LVM_INSERTITEM                  (LVM_FIRST + 77)
#define LVM_FINDITEM                    (LVM_FIRST + 83)
#define LVM_GETSTRINGWIDTH              (LVM_FIRST + 87)
#define LVM_INSERTCOLUMN                (LVM_FIRST + 97)
#define LVM_GETITEMTEXT                 (LVM_FIRST + 115)

#define LVS_EX_CHECKBOXES               0x00000004

#define LVIF_TEXT                       0x00000001
#define LVIF_PARAM                      0x00000004

#define LVIS_STATEIMAGEMASK             0xF000

#define LVNI_SELECTED                   0x0002

#define LVSCW_AUTOSIZE_USEHEADER        -2


#define CBS_DROPDOWN    0x0002L

#define CB_ADDSTRING    0x0143
#define CB_GETCURSEL    0x0147
#define CB_GETITEMDATA  0x0150
#define CB_SETITEMDATA  0x0151




#define BM_SETCHECK     0x00F1

#define BST_CHECKED     0x0001
#define BST_UNCHECKED   0x0000


#define NM_FIRST                (0U-  0U)
#define NM_CLICK                (NM_FIRST-2)


#define INDEXTOSTATEIMAGEMASK(i) ((i) << 12)





//
// Service Control Manager object specific access types
//
#define SC_MANAGER_CONNECT             0x0001
#define SC_MANAGER_CREATE_SERVICE      0x0002
#define SC_MANAGER_ENUMERATE_SERVICE   0x0004
#define SC_MANAGER_LOCK                0x0008
#define SC_MANAGER_QUERY_LOCK_STATUS   0x0010
#define SC_MANAGER_MODIFY_BOOT_CONFIG  0x0020

#define SC_MANAGER_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED      | \
                                        SC_MANAGER_CONNECT            | \
                                        SC_MANAGER_CREATE_SERVICE     | \
                                        SC_MANAGER_ENUMERATE_SERVICE  | \
                                        SC_MANAGER_LOCK               | \
                                        SC_MANAGER_QUERY_LOCK_STATUS  | \
                                        SC_MANAGER_MODIFY_BOOT_CONFIG)


//
// Service object specific access type
//
#define SERVICE_QUERY_CONFIG           0x0001
#define SERVICE_CHANGE_CONFIG          0x0002
#define SERVICE_QUERY_STATUS           0x0004
#define SERVICE_ENUMERATE_DEPENDENTS   0x0008
#define SERVICE_START                  0x0010
#define SERVICE_STOP                   0x0020
#define SERVICE_PAUSE_CONTINUE         0x0040
#define SERVICE_INTERROGATE            0x0080
#define SERVICE_USER_DEFINED_CONTROL   0x0100

#define SERVICE_ALL_ACCESS             (STANDARD_RIGHTS_REQUIRED     | \
                                        SERVICE_QUERY_CONFIG         | \
                                        SERVICE_CHANGE_CONFIG        | \
                                        SERVICE_QUERY_STATUS         | \
                                        SERVICE_ENUMERATE_DEPENDENTS | \
                                        SERVICE_START                | \
                                        SERVICE_STOP                 | \
                                        SERVICE_PAUSE_CONTINUE       | \
                                        SERVICE_INTERROGATE          | \
                                        SERVICE_USER_DEFINED_CONTROL)

//
// Service Types (Bit Mask)
//
#define SERVICE_KERNEL_DRIVER          0x00000001

//
// Start Type
//

#define SERVICE_DEMAND_START           0x00000003

//
// Error control type
//
#define SERVICE_ERROR_NORMAL           0x00000001

#define SERVICE_NO_CHANGE   0xffffffff

#define SERVICE_STOPPED     0x00000001


#define PROCESS_CREATE_THREAD       (0x0002)
#define PROCESS_VM_OPERATION        (0x0008)
#define PROCESS_VM_READ             (0x0010)
#define PROCESS_VM_WRITE            (0x0020)
#define PROCESS_QUERY_INFORMATION   (0x0400)
#define PROCESS_SUSPEND_RESUME      (0x0800)


#define THREAD_QUERY_INFORMATION    (0x0040)
#define THREAD_SET_INFORMATION      (0x0020)


#define PAGE_EXECUTE_READWRITE 0x40
#define ERROR_SUCCESS                    0L


/* NTSTATUS values */
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004
#define STATUS_BUFFER_TOO_SMALL     0xC0000023

#define FILE_LIST_DIRECTORY   0x00000001
#define FILE_ATTRIBUTE_REPARSE_POINT   0x00000400
#define FILE_SHARE_DELETE   0x00000004
#define FILE_OPEN_FOR_BACKUP_INTENT   0x00004000
#define HEAP_CREATE_ENABLE_EXECUTE   0x00040000
//#define HEAP_GENERATE_EXCEPTIONS 0x00000004
#define FIND_FIRST_EX_CASE_SENSITIVE 1
#define FILE_DIRECTORY_FILE 0x00000001
#define FIND_DATA_SIZE   0x00000268


typedef INT_B (__stdcall *FARPROC)();


typedef DWORD (__stdcall *PTHREAD_START_ROUTINE)( VOID *lpThreadParameter );


typedef struct _SECURITY_ATTRIBUTES {
    DWORD   nLength;
    VOID            *lpSecurityDescriptor;
    int             bInheritHandle;
} SECURITY_ATTRIBUTES;

typedef struct _LUID {
  DWORD LowPart;
  INT32  HighPart;
} LUID;

typedef struct _LUID_AND_ATTRIBUTES {
  LUID  Luid;
  DWORD Attributes;
} LUID_AND_ATTRIBUTES;
#define ANYSIZE_ARRAY 1
typedef struct _TOKEN_PRIVILEGES {
  DWORD               PrivilegeCount;
  LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY];
} TOKEN_PRIVILEGES;


typedef struct _SERVICE_STATUS {
    DWORD   dwServiceType;
    DWORD   dwCurrentState;
    DWORD   dwControlsAccepted;
    DWORD   dwWin32ExitCode;
    DWORD   dwServiceSpecificExitCode;
    DWORD   dwCheckPoint;
    DWORD   dwWaitHint;
} SERVICE_STATUS;


typedef struct _QUERY_SERVICE_CONFIG {
    DWORD   dwServiceType;
    DWORD   dwStartType;
    DWORD   dwErrorControl;
    WCHAR*  lpBinaryPathName;
    WCHAR*  lpLoadOrderGroup;
    DWORD   dwTagId;
    WCHAR*  lpDependencies;
    WCHAR*  lpServiceStartName;
    WCHAR*  lpDisplayName;
} QUERY_SERVICE_CONFIG;


typedef struct _LVCOLUMNW
{
    UINT32 mask;
    int fmt;
    int cx;
    WCHAR* pszText;
    int cchTextMax;
    int iSubItem;

    int iImage;
    int iOrder;


    int cxMin;
    int cxDefault;
    int cxIdeal;
} LVCOLUMN;


typedef struct _LVITEMW
{
    DWORD   Mask;
    INT32   Item;
    INT32   SubItem;
    UINT32  State;
    UINT32  StateMask;
    WCHAR*  Text;
    INT32   TextMax;
    INT32   iImage;
    LONG    Param;
    INT32   iIndent;
    INT32   iGroupId;
    DWORD   cColumns;
    DWORD*  puColumns;
    INT32*  piColFmt;
    INT32   iGroup;
} LVITEM;


typedef struct _NMHDR {
  VOID  *hwndFrom;
  DWORD idFrom;
  DWORD code;
} NMHDR;


typedef struct _LV_DISPINFO {
  NMHDR hdr;
  LVITEM item;
} LV_DISPINFO;


typedef struct _GUID {
    DWORD   Data1;
    WORD    Data2;
    WORD    Data3;
    BYTE    Data4[ 8 ];
} GUID;


typedef struct _RECT
{
    SDWORD left;
    SDWORD top;
    SDWORD right;
    SDWORD bottom;
} RECT;


typedef struct _POINT
{
    SDWORD  x;
    SDWORD  y;
} POINT;


typedef struct _NOTIFYICONDATAA {
    DWORD   cbSize;
    HANDLE  NotifyWindowHandle;
    DWORD   uID;
    DWORD   Flags;
    DWORD   uCallbackMessage;
    VOID*   hIcon;
    WCHAR   szTip[128];
    DWORD   dwState;
    DWORD   dwStateMask;
    WCHAR   szInfo[256];
    union {
        UINT32 uTimeout;
        UINT32 uVersion;
    };
    WCHAR   szInfoTitle[64];
    DWORD   dwInfoFlags;
    GUID    guidItem;
    VOID*   hBalloonIcon;
} NOTIFYICONDATA;


typedef struct _MSG {
    VOID    *hwnd;
    DWORD   message;
    DWORD   wParam;
    SDWORD  lParam;
    DWORD   time;
    POINT   pt;
} MSG;


typedef enum _SECURITY_IMPERSONATION_LEVEL {
  SecurityAnonymous,
  SecurityIdentification,
  SecurityImpersonation,
  SecurityDelegation
} SECURITY_IMPERSONATION_LEVEL;
typedef unsigned char SECURITY_CONTEXT_TRACKING_MODE, *PSECURITY_CONTEXT_TRACKING_MODE;
typedef struct _SECURITY_QUALITY_OF_SERVICE {
  unsigned long Length;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  SECURITY_CONTEXT_TRACKING_MODE ContextTrackingMode;
  unsigned char EffectiveOnly;
} SECURITY_QUALITY_OF_SERVICE;


typedef struct _FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME;


typedef struct _WIN32_FIND_DATA {
  DWORD     dwFileAttributes;
  FILETIME  ftCreationTime;
  FILETIME  ftLastAccessTime;
  FILETIME  ftLastWriteTime;
  UINT32    nFileSizeHigh;
  UINT32    nFileSizeLow;
  DWORD     dwReserved0;
  DWORD     dwReserved1;
  WCHAR     cFileName[260];
  WCHAR     cAlternateFileName[14];
} WIN32_FIND_DATA;


typedef struct _REPARSE_DATA_BUFFER {
    DWORD  ReparseTag;
    UINT16 ReparseDataLength;
    UINT16 Reserved;
    union {
        struct {
            UINT16  SubstituteNameOffset;
            UINT16  SubstituteNameLength;
            UINT16  PrintNameOffset;
            UINT16  PrintNameLength;
            wchar_t PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            UINT16  SubstituteNameOffset;
            UINT16  SubstituteNameLength;
            UINT16  PrintNameOffset;
            UINT16  PrintNameLength;
            wchar_t PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            BYTE  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER;


typedef struct _OVERLAPPED {
    DWORD Internal;
    DWORD InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        } DUMMYSTRUCTNAME;
        VOID *Pointer;
    } DUMMYUNIONNAME;

    VOID            *hEvent;
} OVERLAPPED;


typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG HighPart;
    } DUMMYSTRUCTNAME;

    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;

    INT64 QuadPart;

} LARGE_INTEGER;


typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } DUMMYSTRUCTNAME;
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    QWORD QuadPart;
} ULARGE_INTEGER;


typedef enum _MEDIA_TYPE {
    Unknown,
    F5_1Pt2_512,
    F3_1Pt44_512,
    F3_2Pt88_512,
    F3_20Pt8_512,
    F3_720_512,
    F5_360_512,
    F5_320_512,
    F5_320_1024,
    F5_180_512,
    F5_160_512,
    RemovableMedia,
    FixedMedia,
    F3_120M_512,
    F3_640_512,
    F5_640_512,
    F5_720_512,
    F3_1Pt2_512,
    F3_1Pt23_1024,
    F5_1Pt23_1024,
    F3_128Mb_512,
    F3_230Mb_512,
    F8_256_128,
    F3_200Mb_512,
    F3_240M_512,
    F3_32M_512
} MEDIA_TYPE;


typedef struct _DISK_GEOMETRY {
    LARGE_INTEGER   Cylinders;
    MEDIA_TYPE      MediaType;
    DWORD   TracksPerCylinder;
    DWORD   SectorsPerTrack;
    DWORD   BytesPerSector;
} DISK_GEOMETRY;


typedef struct _SP_DEVINFO_DATA {
    DWORD cbSize;
    GUID  ClassGuid;
    DWORD DevInst;    // DEVINST handle
    UINT32 Reserved;
} SP_DEVINFO_DATA;


typedef struct _SP_DEVICE_INTERFACE_DATA {
    DWORD cbSize;
    GUID  InterfaceClassGuid;
    DWORD Flags;
    UINT32 Reserved;
} SP_DEVICE_INTERFACE_DATA;


typedef struct _SP_DEVICE_INTERFACE_DETAIL_DATA_W {
    DWORD  cbSize;
    WCHAR  DevicePath[ANYSIZE_ARRAY];
} SP_DEVICE_INTERFACE_DETAIL_DATA_W;


#define NT_SUCCESS(Status) ((long)(Status) >= 0)
#define FILE_READ_ATTRIBUTES      ( 0x0080 )
#define SECURITY_DYNAMIC_TRACKING   (TRUE)

typedef enum _EXCEPTION_DISPOSITION
{
         ExceptionContinueExecution = 0,
         ExceptionContinueSearch = 1,
         ExceptionNestedException = 2,
         ExceptionCollidedUnwind = 3
} EXCEPTION_DISPOSITION;


typedef struct _EXCEPTION_REGISTRATION_RECORD EXCEPTION_REGISTRATION_RECORD;
typedef struct _EXCEPTION_REGISTRATION_RECORD
{
     EXCEPTION_REGISTRATION_RECORD *Next;
     EXCEPTION_DISPOSITION *Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef struct _NT_TIB NT_TIB;
typedef struct _NT_TIB
{
     EXCEPTION_REGISTRATION_RECORD *ExceptionList;
     VOID *StackBase;
     VOID *StackLimit;
     VOID *SubSystemTib;
     union
     {
          VOID *FiberData;
          UINT32 Version;
     };
     VOID *ArbitraryUserPointer;
     NT_TIB *Self;
} NT_TIB;


typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY  *Flink;
  struct _LIST_ENTRY  *Blink;
} LIST_ENTRY;


typedef struct _RTL_DRIVE_LETTER_CURDIR {

    UINT16 Flags;
    UINT16 Length;
    UINT32 TimeStamp;
    UNICODE_STRING DosPath;

} RTL_DRIVE_LETTER_CURDIR;


typedef struct _RTL_USER_PROCESS_PARAMETERS {

    UINT32 MaximumLength;
    UINT32 Length;
    UINT32 Flags;
    UINT32 DebugFlags;
    VOID *ConsoleHandle;
    UINT32 ConsoleFlags;
    VOID *StdInputHandle;
    VOID *StdOutputHandle;
    VOID *StdErrorHandle;
    UNICODE_STRING CurrentDirectoryPath;
    VOID *CurrentDirectoryHandle;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    VOID *Environment;
    UINT32 StartingPositionLeft;
    UINT32 StartingPositionTop;
    UINT32 Width;
    UINT32 Height;
    UINT32 CharWidth;
    UINT32 CharHeight;
    UINT32 ConsoleTextAttributes;
    UINT32 WindowFlags;
    UINT32 ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopName;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];

} RTL_USER_PROCESS_PARAMETERS;

        typedef void (*PPEBLOCKROUTINE)(
    VOID *PebLock
    );

typedef struct _PEB_FREE_BLOCK PEB_FREE_BLOCK;
typedef struct _PEB_FREE_BLOCK {

    PEB_FREE_BLOCK *Next;
    UINT32 Size;

} PEB_FREE_BLOCK;


typedef struct _SYSTEM_BASIC_INFORMATION
{
    UINT32 Reserved;
    UINT32 TimerResolution;
    UINT32 PageSize;
    UINT32 NumberOfPhysicalPages;
    UINT32 LowestPhysicalPageNumber;
    UINT32 HighestPhysicalPageNumber;
    UINT32 AllocationGranularity;
    UINT_B MinimumUserModeAddress;
    UINT_B MaximumUserModeAddress;
    UINT_B ActiveProcessorsAffinityMask;
    UINT8 NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _THREAD_CYCLE_TIME_INFORMATION {

ULARGE_INTEGER AccumulatedCycles;
ULARGE_INTEGER CurrentCycleCount;

} THREAD_CYCLE_TIME_INFORMATION;


typedef struct _ACL {
    BYTE  AclRevision;
    BYTE  Sbz1;
    WORD   AclSize;
    WORD   AceCount;
    WORD   Sbz2;
} ACL;


typedef enum _SE_OBJECT_TYPE
{
    SE_UNKNOWN_OBJECT_TYPE = 0,
    SE_FILE_OBJECT,
    SE_SERVICE,
    SE_PRINTER,
    SE_REGISTRY_KEY,
    SE_LMSHARE,
    SE_KERNEL_OBJECT,
    SE_WINDOW_OBJECT,
    SE_DS_OBJECT,
    SE_DS_OBJECT_ALL,
    SE_PROVIDER_DEFINED_OBJECT,
    SE_WMIGUID_OBJECT,
    SE_REGISTRY_WOW64_32KEY,
} SE_OBJECT_TYPE;


typedef enum _FINDEX_INFO_LEVELS {
  FindExInfoStandard,
  FindExInfoBasic,
  FindExInfoMaxInfoLevel
} FINDEX_INFO_LEVELS;


typedef enum _FINDEX_SEARCH_OPS {
  FindExSearchNameMatch,
  FindExSearchLimitToDirectories,
  FindExSearchLimitToDevices
} FINDEX_SEARCH_OPS;


typedef struct _RTL_CRITICAL_SECTION_DEBUG
{
    UINT16 Type;
    UINT16 CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Spare[2];
} RTL_CRITICAL_SECTION_DEBUG;


typedef struct _RTL_CRITICAL_SECTION
{
    RTL_CRITICAL_SECTION_DEBUG* DebugInfo;
    INT32                       LockCount;
    INT32                       RecursionCount;
    VOID*                       OwningThread;
    VOID*                       LockSemaphore;
    UINT_B                SpinCount;
} RTL_CRITICAL_SECTION;


#pragma pack(push,1)
typedef struct _FIND_DATA_EX_HANDLE
{
    VOID*                   DirHandle;          //0x00
    VOID*                   Buffer2;            //0x04
    VOID*                   Buffer;             //0x08
    UINT32                  Size;               //0x0C
    UINT32                  dummy4;             //0x10
    FINDEX_INFO_LEVELS      InfoLevel;          //0x14
    DWORD                   AdditionalFlags;    //0x18
    RTL_CRITICAL_SECTION    Lock;               //0x1C

} FIND_DATA_EX_HANDLE;
#pragma pack(pop)


typedef struct _FILE_FULL_DIR_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION;


typedef struct _FILE_BOTH_DIR_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  UINT8 ShortNameLength;
  WCHAR ShortName[12];
  WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION;


typedef union _DIR_INFORMATION
{
    VOID*                       DirInfo;
    FILE_FULL_DIR_INFORMATION*  FullDirInfo;
    FILE_BOTH_DIR_INFORMATION*  BothDirInfo;
} DIR_INFORMATION;

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation         = 1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileObjectIdInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileAttributeTagInformation,
    FileTrackingInformation,
    FileIdBothDirectoryInformation,
    FileIdFullDirectoryInformation,
    FileValidDataLengthInformation,
    FileShortNameInformation,
    FileMaximumInformation
} FILE_INFORMATION_CLASS;

typedef struct _RTLP_CURDIR_REF
{
    LONG    RefCount;
    VOID*   Handle;
} RTLP_CURDIR_REF;

typedef struct _RTL_RELATIVE_NAME_U
{
    UNICODE_STRING      RelativeName;
    VOID*               ContainingDirectory;
    RTLP_CURDIR_REF*    CurDirRef;
} RTL_RELATIVE_NAME_U;


#include "ntuser.h"
#include "slntrtl.h"
#include "ntkeapi.h"
#include "ntexapi.h"
#include "ntpsapi.h"
#include "slntdef.h"
#include "slntpebteb.h"
#include "slntstatus.h"
#include "slntnls.h"
#include "ntbase.h"
#include "ntioapi.h"
#include "slcommctrl.h"
#include "ntobapi.h"
#include "ntrtl.h"
#include "ntregapi.h"

#endif // _WIN_H

