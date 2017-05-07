#include <sl.h>
#include <string.h>

#include "push.h"
#include "file.h"


#define FIELD_OFFSET(type, field)    ((LONG)(UINT_B)&(((type *)0)->field))
#define PTR_ADD_OFFSET(Pointer, Offset) ((VOID*)((UINT_B)(Pointer) + (UINT_B)(Offset)))


BOOLEAN
SymLinkTargetCmp( WCHAR *Name, WCHAR *dest )
{
    VOID    *fileHandle;
    WCHAR   szName[1024];
    BYTE    reparseBuffer[17000];
    BYTE    *reparseData;
    BOOLEAN    bDirectory = FALSE;
    DWORD   dwFlagsAndAttributes;
    IO_STATUS_BLOCK isb;
    NTSTATUS status;

    REPARSE_DATA_BUFFER *reparseInfo = (REPARSE_DATA_BUFFER *) reparseBuffer;

    if (File_GetAttributes(dest) & FILE_ATTRIBUTE_DIRECTORY)
    {
        bDirectory = TRUE;
    }

    if (bDirectory)
    {
        dwFlagsAndAttributes = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;
    }
    else
    {
        dwFlagsAndAttributes = FILE_FLAG_OPEN_REPARSE_POINT;
    }

    status = File_Create(
                &fileHandle,
                Name,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL
                );

    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    status = NtDeviceIoControlFile(
                fileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                FSCTL_GET_REPARSE_POINT,
                reparseInfo,
                sizeof(reparseBuffer),
                NULL,
                0
                );

    reparseData = (BYTE *) &reparseInfo->SymbolicLinkReparseBuffer.PathBuffer;

    String_CopyN(
        szName,
        (WCHAR *) (reparseData + reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameOffset),
        reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength );

    szName[reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength] = 0;

    NtClose(fileHandle);

    if (String_CompareN(dest, szName + 6, wcslen(dest)) != 0)
        return FALSE;
    else
        return TRUE;
}


VOID
CreateLink( WCHAR *name, WCHAR *dest )
{
    if (!SymLinkTargetCmp( name, dest ))
    {
        if (File_GetAttributes(dest) & FILE_ATTRIBUTE_DIRECTORY)
        {
            RemoveDirectoryW(name);

            CreateSymbolicLinkW(name, dest, TRUE);
        }
        else
        {
            File_Delete(name);

            CreateSymbolicLinkW(name, dest, FALSE);
        }
    }
}


WCHAR* GetPointerToFilePath( WCHAR *Path, WCHAR *File )
{
    WCHAR *filePath;

    filePath = (WCHAR *) Memory_Allocate((wcslen(Path) + wcslen(File) + 2) * 2);

    String_Copy(filePath, Path);

    String_Concatenate(filePath, L"\\");
    String_Concatenate(filePath, File);

    return filePath;
}


#define PROGRESS_CONTINUE   0


VOID MarkForCache( WCHAR *FilePath )
{
    WCHAR *newPath, *pszFileName;


    pszFileName = String_FindLastChar(FilePath, '\\') + 1;
    newPath = (WCHAR*) Memory_Allocate( (String_GetLength(FilePath) + 7) * 2 );

    GetPathOnly(FilePath, newPath);

    String_Concatenate(newPath, L"cache_");
    String_Concatenate(newPath, pszFileName);

    File_Rename(FilePath, newPath);
}


BOOLEAN FolderExists( WCHAR* Folder )
{
    NTSTATUS status;
    VOID *directoryHandle;

    status = File_Create(
                &directoryHandle,
                Folder,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE,
                NULL
                );

    if (NT_SUCCESS(status))
    {
        NtClose(directoryHandle);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


#define MEM_RESERVE 0x2000

#define STATUS_END_OF_FILE              ((NTSTATUS)0xC0000011L)
#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_WRITE_ATTRIBUTES        0x00000100
#define STATUS_NO_MEMORY                 ((DWORD)0xC0000017)
#define STATUS_COMMITMENT_LIMIT                 ((DWORD)0xC000012D)

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;
typedef struct _FILE_BASIC_INFORMATION
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;
NTSTATUS __stdcall NtQueryFullAttributesFile(
    OBJECT_ATTRIBUTES* ObjectAttributes,
    FILE_NETWORK_OPEN_INFORMATION* FileInformation
);


/**
* Creates or opens a file.
*
* \param FileHandle A variable that receives the file handle.
* \param FileName The Win32 file name.
* \param DesiredAccess The desired access to the file.
* \li \c FILE_READ_ATTRIBUTES | GENERIC_READ | SYNCHRONIZE Grants read
* access.
* \li \c FILE_READ_ATTRIBUTES | GENERIC_WRITE | SYNCHRONIZE Grants
* write access.
* \li \c DELETE | SYNCHRONIZE Allows you to rename file.
* \param ShareAccess The file access granted to other threads.
* \li \c FILE_SHARE_READ Allows other threads to read from the file.
* \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
* \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
* \note: To rename a file use FILE_SHARE_READ | FILE_SHARE_WRITE |
* FILE_SHARE_DELETE
* \param CreateDisposition The action to perform if the file does or
* does not exist.
* \li \c FILE_OPEN If the file exists, open it. Otherwise, fail.
* \li \c FILE_OVERWRITE_IF If the file exists, open and overwrite it.
* Otherwise, create the file.
* \param CreateOptions The options to apply when the file is opened or
* created.
* \li \c FILE_DIRECTORY_FILE Use for directory.
* \li \c FILE_NON_DIRECTORY_FILE Use for non-directory.
*/


NTSTATUS File_Create(
    VOID** FileHandle,
    WCHAR* FileName,
    DWORD DesiredAccess,
    DWORD ShareAccess,
    DWORD CreateDisposition,
    DWORD CreateOptions,
    DWORD* CreateStatus
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrib;
    IO_STATUS_BLOCK ioStatusBlock;
    VOID *fileHandle;

    if(!RtlDosPathNameToNtPathName_U(
        FileName,
        &fileName,
        NULL,
        NULL
        ))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = NULL;
    objAttrib.ObjectName = &fileName;
    objAttrib.Attributes = OBJ_CASE_INSENSITIVE;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objAttrib,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    Memory_Free(fileName.Buffer);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = ioStatusBlock.Information;

    return status;
}


HANDLE File_Open( WCHAR* FileName, DWORD DesiredAccess )
{
    HANDLE fileHandle = NULL;

    File_Create(
        &fileHandle,
        FileName,
        DesiredAccess,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    return fileHandle;
}


/**
* Checks if a file exists.
*
* \param FileName The Win32 file name.
*/

BOOLEAN File_Exists( WCHAR* FileName )
{
    NTSTATUS status;
    VOID *fileHandle;

    status = File_Create(
        &fileHandle,
        FileName,
        FILE_READ_ATTRIBUTES |GENERIC_READ | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        NtClose(fileHandle);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/**
* Copies an existing file to a new file.
*
* \param SourceFileName The Win32 file name of source file.
* \param DestinationFileName The Win32 file name for the new
* file.
* \param ProgressRoutine The address of a callback function of
* type TYPE_FsProgessRoutine that is called each time another
* portion of the file has been copied. This parameter can be
* NULL if no progress routine is required.
*/

VOID File_Copy(
    WCHAR* SourceFileName,
    WCHAR* DestinationFileName,
    TYPE_FsProgessRoutine ProgressRoutine
    )
{
    VOID *fileHandleSource = NULL, *fileHandleDest = NULL;
    UCHAR *buffer = NULL;
    UINT_B regionSize = 0x10000;
    NTSTATUS status;
    UINT64 fileSize, bytesCopied = 0;
    IO_STATUS_BLOCK isb;

    status = File_Create(
        &fileHandleSource,
        SourceFileName,
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return;

    status = File_Create(
        &fileHandleDest,
        DestinationFileName,
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_WRITE,
        FILE_SHARE_WRITE,
        FILE_CREATE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_OBJECT_PATH_NOT_FOUND)
        {
            //TODO: Implement with CreatePath() function.
            HANDLE directoryHandle;
            WCHAR directoryName[260];
            WCHAR *end;

            end = String_FindFirstChar(DestinationFileName, L'\\');

            while (end != NULL)
            {
                String_CopyN(directoryName, DestinationFileName, end - DestinationFileName + 1);

                status = File_Create(
                    &directoryHandle,
                    directoryName,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_CREATE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                    NULL
                    );

                end = String_FindFirstChar(++end, L'\\');
            }

            //try again.
            status = File_Create(
                &fileHandleDest,
                DestinationFileName,
                SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_WRITE,
                FILE_SHARE_WRITE,
                FILE_CREATE,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL
                );

            //still fails? abort.
            if (!NT_SUCCESS(status))
            {
                return;
            }
        }
        else
        {
            return;
        }
    }

    if (ProgressRoutine)
    {
        //get file size
        FILE_STANDARD_INFORMATION fileInformation;

        NtQueryInformationFile(
            fileHandleSource,
            &isb,
            &fileInformation,
            sizeof(FILE_STANDARD_INFORMATION),
            FileStandardInformation
            );

        fileSize = fileInformation.EndOfFile.QuadPart;
    }

    NtAllocateVirtualMemory(
        NtCurrentProcess(),
        (VOID**)&buffer,
        0,
        &regionSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
        );

    while (TRUE)
    {
        status = NtReadFile(
            fileHandleSource,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            regionSize,
            NULL,
            NULL
            );

        if (status == STATUS_END_OF_FILE)
        {
            break;
        }
        else
        {
            NtWriteFile(
                fileHandleDest,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                isb.Information,
                NULL,
                NULL
                );

            bytesCopied += isb.Information;
        }

        if (ProgressRoutine)
            ProgressRoutine(fileSize, bytesCopied);
    }

    NtFreeVirtualMemory(NtCurrentProcess(), (VOID**)&buffer, &regionSize, MEM_RELEASE);

    if (fileHandleSource)
    {
        NtClose(fileHandleSource);
    }

    if (fileHandleDest)
    {
        NtClose(fileHandleDest);
    }
}


VOID CreatePath( WCHAR* Path )
{
    HANDLE directoryHandle;
    WCHAR directoryName[260];
    WCHAR *end;

    end = String_FindFirstChar(Path, L'\\');

    while (end != NULL)
    {
        String_CopyN(directoryName, Path, end - Path + 1);

        directoryName[end - Path + 1] = L'\0';

        File_Create(
            &directoryHandle,
            directoryName,
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_CREATE,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
            NULL
            );

        end = String_FindFirstChar(++end, L'\\');
    }
}


VOID File_Split( HANDLE InputFile, WCHAR* FileName, DWORD Offset, DWORD Size, WCHAR* OutputPath )
{
    HANDLE outputFile = NULL;
    WCHAR fileName[60];
    VOID *buffer = NULL;
    UINT_B regionSize;
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    UINT32 bytesCopied = 0;
    UINT32 bytesRemaining = 0;

    String_Copy(fileName, OutputPath);
    String_Concatenate(fileName, FileName);

    status = File_Create(
        &outputFile,
        fileName,
        FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        CreatePath(OutputPath);

        //try again.
        File_Create(
            &outputFile,
            fileName,
            FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA | SYNCHRONIZE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL
            );
    }

    regionSize = Size;

    status = NtAllocateVirtualMemory(
        NtCurrentProcess(),
        (VOID**)&buffer,
        0,
        &regionSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
        );

    if (status == STATUS_NO_MEMORY)
    {
        SYSTEM_BASIC_INFORMATION basicInfo;
        SYSTEM_BASIC_PERFORMANCE_INFORMATION performanceInfo;
        UINT8 percentageOfMemory = 90;

        NtQuerySystemInformation(
            SystemBasicInformation,
            &basicInfo,
            sizeof(SYSTEM_BASIC_INFORMATION),
            0
            );

        // Check available memory
        NtQuerySystemInformation(
            SystemBasicPerformanceInformation,
            &performanceInfo,
            sizeof(SYSTEM_BASIC_PERFORMANCE_INFORMATION),
            NULL
            );

        while (status == STATUS_NO_MEMORY || status == STATUS_COMMITMENT_LIMIT)
        {
            regionSize = performanceInfo.AvailablePages * basicInfo.PageSize;
            regionSize = (FLOAT)((FLOAT)regionSize / (FLOAT)100) * percentageOfMemory;

            status = NtAllocateVirtualMemory(
                NtCurrentProcess(),
                (VOID**)&buffer,
                0,
                &regionSize,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE
                );

            percentageOfMemory -= 10;
        }

        if (!buffer)
        {
            return;
        }
    }

    File_SetPointer(InputFile, Offset, FILE_BEGIN);

    while (bytesCopied < Size)
    {
        status = NtReadFile(
            InputFile,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            bytesRemaining < regionSize ? bytesRemaining : regionSize,
            NULL,
            NULL
            );

        status = NtWriteFile(
            outputFile,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            isb.Information,
            NULL,
            NULL
            );

        bytesCopied += isb.Information;
        bytesRemaining = Size - bytesCopied;
    }

    NtFreeVirtualMemory(
        NtCurrentProcess(),
        (VOID**)&buffer,
        &regionSize,
        MEM_RELEASE
        );

    if (outputFile)
        NtClose(outputFile);
}


/**
* Loads a file into memory and returns the base address.
*
* \param FileName The Win32 file name.
* \param FileSize Optional, returns the file size.
*/

VOID* File_Load( WCHAR* FileName, UINT64* FileSize )
{
    HANDLE fileHandle;
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    FILE_STANDARD_INFORMATION fileInformation;
    UINT64 fileSize;
    VOID *buffer;

    status = File_Create(
        &fileHandle,
        FileName,
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_READ,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return NULL;

    // Get file size
    NtQueryInformationFile(
        fileHandle,
        &isb,
        &fileInformation,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
        );

    fileSize = fileInformation.EndOfFile.QuadPart;

    // If the user wants it, give it to him
    if (FileSize)
        *FileSize = fileSize;

    // Allocate some memory
    buffer = Memory_Allocate(fileSize);

    // Read the entire file into memory
    status = NtReadFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        buffer,
        fileSize,
        NULL,
        NULL
        );

    // We got what we need, the file handle is no longer needed.
    NtClose(fileHandle);

    return buffer;
}


/**
* Retrieves the size of a file.
*
* \param FileName The Win32 file name.
*/

UINT64 File_GetSize( WCHAR* FileName )
{
    VOID *fileHandle;
    IO_STATUS_BLOCK isb;
    FILE_STANDARD_INFORMATION fileInformation;
    NTSTATUS status;

    status = File_Create(
        &fileHandle,
        FileName,
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_READ,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return 0;

    NtQueryInformationFile(
        fileHandle,
        &isb,
        &fileInformation,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
        );

    NtClose(fileHandle);

    return fileInformation.EndOfFile.QuadPart;
}


BOOLEAN File_GetLastWriteTime( HANDLE FileHandle, FILETIME* LastWriteTime )
{
    IO_STATUS_BLOCK isb;
    FILE_BASIC_INFORMATION fileInformation;

    NtQueryInformationFile(
        FileHandle, 
        &isb, 
        &fileInformation, 
        sizeof(FILE_BASIC_INFORMATION), 
        FileBasicInformation
        );

    LastWriteTime->dwLowDateTime = fileInformation.LastWriteTime.u.LowPart;
    LastWriteTime->dwHighDateTime = fileInformation.LastWriteTime.u.HighPart;

    return TRUE;
}


DWORD File_GetAttributes( WCHAR* FileName )
{
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrib;
    FILE_NETWORK_OPEN_INFORMATION fileInformation;

    RtlDosPathNameToNtPathName_U(FileName, &fileName, NULL, NULL);

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = NULL;
    objAttrib.ObjectName = &fileName;
    objAttrib.Attributes = OBJ_CASE_INSENSITIVE;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    NtQueryFullAttributesFile(&objAttrib, &fileInformation);

    return fileInformation.FileAttributes;
}


UINT32 File_Read( HANDLE FileHandle, VOID* Buffer, UINT32 Length )
{
    IO_STATUS_BLOCK isb;

    NtReadFile(FileHandle, NULL, NULL, NULL, &isb, Buffer, Length, NULL, NULL);

    return isb.Information;
}


VOID File_Write( HANDLE FileHandle, VOID* Buffer, UINT32 Length )
{
    IO_STATUS_BLOCK isb;

    NtWriteFile(FileHandle, NULL, NULL, NULL, &isb, Buffer, Length, NULL, NULL );
}


INT64 File_GetPointer( HANDLE FileHandle )
{
    FILE_POSITION_INFORMATION positionInfo;
    IO_STATUS_BLOCK isb;

    NtQueryInformationFile(
        FileHandle,
        &isb,
        &positionInfo,
        sizeof(FILE_POSITION_INFORMATION),
        FilePositionInformation
        );

    return positionInfo.CurrentByteOffset.QuadPart;
}


VOID File_SetPointer( HANDLE FileHandle, INT64 DistanceToMove, DWORD MoveMethod )
{
    FILE_POSITION_INFORMATION positionInfo;
    IO_STATUS_BLOCK isb;

    positionInfo.CurrentByteOffset.QuadPart = DistanceToMove;

    NtSetInformationFile(
        FileHandle, 
        &isb, 
        &positionInfo,
        sizeof(FILE_POSITION_INFORMATION),
        FilePositionInformation
        );
}


VOID File_Delete( WCHAR* FileName )
{
    HANDLE fileHandle;
    NTSTATUS status;

    status = File_Create(
        &fileHandle,
        FileName,
        DELETE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DELETE_ON_CLOSE,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        NtClose(fileHandle);
    }
}


/**
* Renames a file.
*
* \param FilePath The Win32 file name.
* \param NewFileName The new file name.
*/

VOID File_Rename( WCHAR* FilePath, WCHAR* NewFileName )
{
    VOID *fileHandle;
    UINT32 renameInfoSize;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_RENAME_INFORMATION *renameInfo;
    UNICODE_STRING newFileName;

    File_Create(
        &fileHandle,
        FilePath,
        DELETE | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    RtlDosPathNameToNtPathName_U(NewFileName, &newFileName, NULL, NULL);

    renameInfoSize = FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName) + (ULONG)newFileName.Length;

    renameInfo = (FILE_RENAME_INFORMATION*)Memory_Allocate(renameInfoSize);
    renameInfo->ReplaceIfExists = FALSE;
    renameInfo->RootDirectory = NULL;
    renameInfo->FileNameLength = (ULONG)newFileName.Length;

    memcpy(renameInfo->FileName, newFileName.Buffer, newFileName.Length);

    NtSetInformationFile(
        fileHandle,
        &ioStatusBlock,
        renameInfo,
        renameInfoSize,
        FileRenameInformation
        );

    Memory_Free(newFileName.Buffer);
    Memory_Free(renameInfo);

    NtClose(fileHandle);
}


VOID File_Close( HANDLE FileHandle )
{
    NtClose(FileHandle);
}
