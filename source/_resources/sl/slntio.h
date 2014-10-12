// Create disposition

#define FILE_OPEN           0x00000001
#define FILE_OVERWRITE_IF   0x00000005

// NtQueryInformationFile/NtSetInformationFile types

typedef struct _FILE_RENAME_INFORMATION
{
    BOOLEAN ReplaceIfExists;
    VOID*   RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_RENAME_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION
{
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION;