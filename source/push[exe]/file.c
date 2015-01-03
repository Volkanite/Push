#include <sltypes.h>
#include <slnt.h>
#include <slntuapi.h>
#include <pushbase.h>
#include <slc.h>
#include <slfile.h>
//#include <slgui.h>
#include <wchar.h>
#include "push.h"
#include "file.h"


#define FIELD_OFFSET(type, field)    ((LONG)(UINT_B)&(((type *)0)->field))
#define PTR_ADD_OFFSET(Pointer, Offset) ((VOID*)((UINT_B)(Pointer) + (UINT_B)(Offset)))







/*BOOLEAN
IsCacheFile( WCHAR *fileName )
{
    WCHAR *slash = SlStringFindLastChar(
                    fileName,
                    '\\'
                    );

    if (slash)
        fileName = slash + 1;

    if ( SlStringCompare(fileName, L"cache_", 6) == 0 )
        return TRUE;
    else
        return FALSE;
}*/


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

    status = SlFileCreate(
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

    SlStringCopy(
        szName,
        (WCHAR *) (reparseData + reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameOffset),
        reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength );

    szName[reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength] = 0;

    NtClose(fileHandle);

    if (SlStringCompare(dest, szName + 6, wcslen(dest)) != 0)
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

    wcscpy(filePath, Path);

    wcscat(filePath, L"\\");
    wcscat(filePath, File);

    return filePath;
}


#define PROGRESS_CONTINUE   0


VOID
MarkForCache( WCHAR *FilePath )
{
    WCHAR *newPath, *pszFileName;


    pszFileName = SlStringFindLastChar(FilePath, '\\') + 1;

    newPath = (WCHAR*) _alloca(
                            (wcslen(FilePath) + 7) * 2
                            );

    GetPathOnly(FilePath, newPath);

    wcscat(newPath, L"cache_");
    wcscat(newPath, pszFileName);

    //PushRenameFile(FilePath, pszNewPath);
    FsRenameFile(FilePath, newPath);
}





BOOLEAN
FolderExists( WCHAR* Folder )
{
    NTSTATUS status;
    VOID *directoryHandle;

    /*file = SlFileCreate(
        Folder,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE
        );

    if (file != INVALID_HANDLE_VALUE)
    {
        NtClose(file);

        return TRUE;
    }
    else
    {
        return FALSE;
    }*/

    status = SlFileCreate(
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
* Enumerates a directory.
*
* \param Directory The Win32 directory name.
* \param SearchPattern Search expression/wildcards.
* \param Callback The address of a callback function of type FS_ENUM_DIRECTORY that is
* called for each file that matches the search expression.
*/

NTSTATUS
FsEnumDirectory( WCHAR* Directory, WCHAR* SearchPattern, FS_ENUM_DIRECTORY Callback )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    BOOLEAN firstTime = TRUE;
    VOID *directoryHandle, *buffer;
    UINT32 bufferSize = 0x400;
    UINT32 i;
    UNICODE_STRING pattern;

    SlFileCreate(
        &directoryHandle,
        Directory,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    buffer = RtlAllocateHeap(PushHeapHandle, 0, bufferSize);

    SlInitUnicodeString(&pattern, SearchPattern);

    while (TRUE)
    {
        // Query the directory, doubling the buffer each time NtQueryDirectoryFile fails.
        while (TRUE)
        {
            status = NtQueryDirectoryFile(
                        directoryHandle,
                        NULL,
                        NULL,
                        NULL,
                        &isb,
                        buffer,
                        bufferSize,
                        FileDirectoryInformation,
                        FALSE,
                        &pattern,
                        firstTime
                        );

            // Our ISB is on the stack, so we have to wait for the operation to complete
            // before continuing.
            if (status == STATUS_PENDING)
            {
                status = NtWaitForSingleObject(directoryHandle, FALSE, NULL);

                if (NT_SUCCESS(status))
                    status = isb.Status;
            }

            if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH)
            {
                RtlFreeHeap(PushHeapHandle, 0, buffer);

                bufferSize *= 2;
                buffer = RtlAllocateHeap(PushHeapHandle, 0, bufferSize);
            }
            else
            {
                break;
            }

        }

        // If we don't have any entries to read, exit.
        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        // Read the batch and execute the callback function for each file.

        i = 0;

        while (TRUE)
        {
            FILE_DIRECTORY_INFORMATION *information;

            information = (FILE_DIRECTORY_INFORMATION *) ( ((UINT_B)(buffer)) + i );

            Callback(Directory, information);

            if (information->NextEntryOffset != 0)
                i += information->NextEntryOffset;
            else
                break;
        }

        firstTime = FALSE;
    }

    RtlFreeHeap(PushHeapHandle, 0, buffer);

    NtClose(directoryHandle);

    return status;
}


/**
* Renames a file.
*
* \param FilePath The Win32 file name.
* \param NewFileName The new file name.
*/

VOID
FsRenameFile( WCHAR* FilePath, WCHAR* NewFileName )
{
    VOID *fileHandle, *heapHandle;
    UINT32 renameInfoSize;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_RENAME_INFORMATION *renameInfo;
    UNICODE_STRING newFileName;

    /*file = SlFileCreate(
            FilePath,
            DELETE | SYNCHRONIZE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );*/

    SlFileCreate(
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


/**
* Retrieves the size of a file.
*
* \param FileName The Win32 file name.
*/

UINT64
FsFileGetSize( WCHAR* FileName )
{
    VOID *fileHandle;
    IO_STATUS_BLOCK isb;
    FILE_STANDARD_INFORMATION fileInformation;
    NTSTATUS status;

    status = SlFileCreate(
                &fileHandle,
                FileName,
                SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_READ,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
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
