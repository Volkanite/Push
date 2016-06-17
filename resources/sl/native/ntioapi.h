// Create disposition

#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE_IF               0x00000005

// Create/open flags

#define FILE_SYNCHRONOUS_IO_NONALERT    0x00000020
#define FILE_NON_DIRECTORY_FILE         0x00000040

typedef struct _IO_STATUS_BLOCK
{
    union
    {
        NTSTATUS Status;
        VOID* Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

// NtQueryInformationFile/NtSetInformationFile types

typedef struct _FILE_STANDARD_INFORMATION
{
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION
{
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION
{
    BOOLEAN ReplaceIfExists;
    VOID*   RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_RENAME_INFORMATION;


// NtQueryDirectoryFile types

typedef struct _FILE_DIRECTORY_INFORMATION
{
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
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION;

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

typedef
VOID
(__stdcall *PIO_APC_ROUTINE) (
    VOID *ApcContext,
    IO_STATUS_BLOCK *IoStatusBlock,
    UINT32 Reserved
    );
#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS __stdcall NtWriteFile(
    HANDLE FileHandle,
    HANDLE Event,
    VOID* ApcRoutine,
    VOID* ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    VOID* Buffer,
    ULONG Length,
    LARGE_INTEGER* ByteOffset,
    ULONG* Key
    );
    
NTSTATUS __stdcall NtDeviceIoControlFile(
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

NTSTATUS __stdcall NtLoadDriver(
    UNICODE_STRING *pDrvName
    );
    
NTSTATUS __stdcall NtQueryDirectoryFile(
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
    
NTSTATUS __stdcall NtCreateFile(
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
    
NTSTATUS __stdcall NtReadFile(
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
    
NTSTATUS __stdcall NtSetInformationFile(
    VOID*                   FileHandle,
    IO_STATUS_BLOCK*        IoStatusBlock,
    VOID*                   FileInformation,
    ULONG                   Length,
    FILE_INFORMATION_CLASS  FileInformationClass
    );
    
NTSTATUS __stdcall NtQueryInformationFile(
    VOID* FileHandle,
    IO_STATUS_BLOCK* IoStatusBlock,
    VOID* FileInformation,
    ULONG Length,
    FILE_INFORMATION_CLASS FileInformationClass
    );
    
 #ifdef __cplusplus
 }
 #endif