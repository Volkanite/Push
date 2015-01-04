// Create disposition

#define FILE_OPEN           0x00000001
#define FILE_CREATE         0x00000002
#define FILE_OPEN_IF        0x00000003
#define FILE_OVERWRITE_IF   0x00000005


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