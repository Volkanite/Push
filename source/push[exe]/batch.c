#include <sl.h>
#include <string.h>

#include "push.h"
#include "file.h"
#include "batch.h"


VOID GetBatchFile( PUSH_GAME* Game, WCHAR* Buffer )
{
    WCHAR *dot;
    WCHAR batchFile[260] = L"cache\\";

    String_Concatenate(batchFile, Game->Name);

    dot = String_FindLastChar(batchFile, '.');

    if (dot)
        String_Copy(dot, L".txt");
    else
        String_Concatenate(batchFile, L".txt");

    String_Copy(Buffer, batchFile);
}


VOID BatchFile_Initialize( PUSH_GAME* Game )
{
    UINT64 fileSize;
    VOID *buffer = NULL, *bufferOffset;
    WCHAR *lineStart, *nextLine, *end;
    FILE_LIST_ENTRY fileEntry;
    WCHAR line[260];
    UINT16 lineLength;
    WCHAR batchFile[260];

    FileList = NULL;

    // Get the batchfile name and path
    GetBatchFile(Game, batchFile);

    // Allocate some memory for the batchfile name
    BatchFileName = (WCHAR*) Memory_Allocate(
		(String_GetLength(batchFile) + 1) * sizeof(WCHAR)
		);

    // Save new batchfile name
    String_Copy(BatchFileName, batchFile);

    // Open the batchfile and read the entire file into memory
    buffer = File_Load(batchFile, &fileSize);

    if (!buffer)
        return;

    // Start our reads after the UTF16-LE character marker
    bufferOffset = (WCHAR*) buffer + 1;

    // Update fileSize to reflect. It is now invalid for use as the
    // actual file size.
    fileSize -= sizeof(WCHAR);

    // Get the end of the buffer
    end = (WCHAR *)((UINT_B)bufferOffset + fileSize);

    // Let's start the day off nicely
    nextLine = (WCHAR*) bufferOffset;
    fileSize = 0;

    // Get all file names and queue them for caching
    while (nextLine < end)
    {
        lineStart = nextLine;

        // Try to find new line character
        nextLine = Memory_FindFirstChar(lineStart, '\n', end - lineStart);

        if (!nextLine)
            // Didn't find it? How about return?
            nextLine = Memory_FindFirstChar(lineStart, '\r', end - lineStart);

        if (!nextLine)
            // Still didn't find any? Must not be one.
            nextLine = end + 1;

        lineLength = (nextLine - 1) - lineStart;

        // Copy line into a buffer
        wcsncpy(line, lineStart, lineLength);

        // Terminate the buffer;
        line[lineLength] = L'\0';

        nextLine++;

        // Add to file list
        fileEntry.Name = line;
        fileEntry.Bytes = File_GetSize(line);

        PushAddToFileList(&FileList, &fileEntry);

        // Increment total batch size counter
        fileSize += fileEntry.Bytes;
    }

    // Be a nice guy and give back what was given to you
    RtlFreeHeap(PushHeapHandle, 0, buffer);

    // Update batch size;
    BatchSize = fileSize;
}


/**
* Checks whether a file is included in a batchfile.
*
* \param BatchFile The batchfile.
* \param File A pointer to a FILE_LIST_ENTRY
* structure representing the file.
*/

BOOLEAN BatchFile_IsBatchedFile( FILE_LIST_ENTRY* File )
{
    FILE_LIST_ENTRY *file;

    file = (FILE_LIST_ENTRY*) FileList;

    while (file != NULL)
    {
        if (File->Bytes == file->Bytes
            && String_Compare(File->Name, file->Name) == 0)
            return TRUE;

        file = file->NextEntry;
    }

    return FALSE;
}


/**
* Gets the size (in bytes) of
* a batchfile.
*
* \param BatchFile The batchfile.
*/

UINT64 BatchFile_GetBatchSize()
{
    return BatchSize;
}


/**
* Saves the batchfile to disk.
*
* \param BatchFile The batchfile.
*/

VOID BatchFile_SaveBatchFile()
{
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK isb;
    FILE_LIST_ENTRY *file;
    WCHAR marker = 0xFEFF;
    WCHAR end[] = L"\r\n";
    NTSTATUS status;

    status = File_Create(
        &fileHandle,
        BatchFileName,
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		NULL
        );

    // Check if \cache folder has not been created yet.

    if (status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        HANDLE directoryHandle;

        File_Create(
            &directoryHandle,
            L"cache",
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_CREATE,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
			NULL
            );

        File_Close(directoryHandle);

        //try again.
        File_Create(
            &fileHandle,
            BatchFileName,
            SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL
            );
    }

    file = (FILE_LIST_ENTRY*) FileList;

    // Write character marker
    NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        &marker,
        sizeof(marker),
        NULL,
        NULL
        );

    // Write all batch items to file
    while (file != 0)
    {
        // Write a single item to batch file
        NtWriteFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            file->Name,
            String_GetLength(file->Name) * sizeof(WCHAR),
            NULL,
            NULL
            );

        // Write new line
        NtWriteFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            &end,
            4,
            NULL,
            NULL
            );

        file = file->NextEntry;
    }

    if (fileHandle)
    {
        File_Close(fileHandle);
    }
}


/**
* Adds an item to a
* batchfile.
*
* \param BatchFile The
* batchfile.
* \param File Pointer to a
* FILE_LIST_ENTRY structure
* with details of the file.
*/

VOID BatchFile_AddItem( FILE_LIST_ENTRY* File )
{
    FILE_LIST_ENTRY file;

    file.Name = File->Name;
    file.Bytes = File->Bytes;

    PushAddToFileList(
        &FileList,
        &file
        );
}


/**
* Removes an item from a batchfile.
*
* \param File Pointer to a FILE_LIST_ENTRY structure with
* details of the file.
*/

VOID BatchFile_RemoveItem( FILE_LIST_ENTRY* File )
{
    FILE_LIST_ENTRY *file, *previousEntry;

    file = FileList;

    while (file != 0)
    {
        if (file->Bytes == File->Bytes
            && String_Compare(file->Name, File->Name) == 0)
        {
            if (file == FileList)
                FileList = file->NextEntry;
            else
                previousEntry->NextEntry = file->NextEntry;

            break;
        }

        previousEntry = file;
        file = file->NextEntry;
    }
}


/**
* Gets a pointer to
* the batchfile list
* used by the
* BatchFile class.
*/

FILE_LIST BatchFile_GetBatchList()
{
    return FileList;
}
