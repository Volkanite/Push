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

    if (GetFileAttributesW(dest) & FILE_ATTRIBUTE_DIRECTORY)
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

    status = File::Create(
                &fileHandle,
                Name,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
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

    String::CopyN(
        szName,
        (WCHAR *) (reparseData + reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameOffset),
        reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength );

    szName[reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength] = 0;

    NtClose(fileHandle);

    if (String::CompareN(dest, szName + 6, wcslen(dest)) != 0)
        return FALSE;
    else
        return TRUE;
}


VOID
CreateLink( WCHAR *name, WCHAR *dest )
{
    if (!SymLinkTargetCmp( name, dest ))
    {
        if (GetFileAttributesW(dest) & FILE_ATTRIBUTE_DIRECTORY)
        {
            RemoveDirectoryW(name);

            CreateSymbolicLinkW(name, dest, TRUE);
        }
        else
        {
            DeleteFileW(name);

            CreateSymbolicLinkW(name, dest, FALSE);
        }
    }
}


WCHAR*
GetPointerToFilePath( WCHAR *Path, WCHAR *File )
{
    WCHAR *filePath;

    //pszFilePath = (WCHAR*) SlAllocate((wcslen(pszPath) + wcslen(pszFile) + 2) * 2);
    filePath = (WCHAR *) RtlAllocateHeap(
                            PushHeapHandle,
                            0,
                            (wcslen(Path) + wcslen(File) + 2) * 2
                            );

    String::Copy(filePath, Path);

    String::Concatenate(filePath, L"\\");
    String::Concatenate(filePath, File);

    return filePath;
}


#define PROGRESS_CONTINUE   0


VOID MarkForCache( WCHAR *FilePath )
{
    WCHAR *newPath, *pszFileName;


    pszFileName = String::FindLastChar(FilePath, '\\') + 1;
    newPath = (WCHAR*) Memory::Allocate( (String::GetLength(FilePath) + 7) * 2 );

    GetPathOnly(FilePath, newPath);

    String::Concatenate(newPath, L"cache_");
    String::Concatenate(newPath, pszFileName);

    FsRenameFile(FilePath, newPath);
}


BOOLEAN FolderExists( WCHAR* Folder )
{
    NTSTATUS status;
    VOID *directoryHandle;

    status = File::Create(
                &directoryHandle,
                Folder,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE
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


/**
* Renames a file.
*
* \param FilePath The Win32 file name.
* \param NewFileName The new file name.
*/

VOID FsRenameFile( WCHAR* FilePath, WCHAR* NewFileName )
{
    VOID *fileHandle, *heapHandle;
    UINT32 renameInfoSize;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_RENAME_INFORMATION *renameInfo;
    UNICODE_STRING newFileName;

    File::Create(
        &fileHandle,
        FilePath,
        DELETE | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    RtlDosPathNameToNtPathName_U(NewFileName, &newFileName, NULL, NULL);

    renameInfoSize = FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName) + (ULONG)newFileName.Length;

    heapHandle = PushHeapHandle;

    renameInfo = (FILE_RENAME_INFORMATION*) RtlAllocateHeap(
                                                heapHandle,
                                                0,
                                                renameInfoSize
                                                );

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

    RtlFreeHeap(heapHandle, 0, newFileName.Buffer);
    RtlFreeHeap(heapHandle, 0, renameInfo);

    NtClose(fileHandle);
}
