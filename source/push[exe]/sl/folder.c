#include <sl.h>


/**
* Enumerates a directory.
*
* \param Directory The Win32 directory name.
* \param SearchPattern Search expression/wildcards.
* \param Callback The address of a callback function of type FS_ENUM_DIRECTORY that is
* called for each file that matches the search expression.
*/

NTSTATUS Directory_Enum(
    WCHAR* Directory,
    WCHAR* SearchPattern,
    SL_ENUM_DIRECTORY Callback
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    BOOLEAN firstTime = TRUE;
    VOID *directoryHandle, *buffer;
    UINT32 bufferSize = 0x400;
    UINT32 i;
    UNICODE_STRING pattern;

    status = File_Create(
        &directoryHandle,
        Directory,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    buffer = Memory_Allocate(bufferSize);

    UnicodeString_Init(&pattern, SearchPattern);

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
                Memory_Free(buffer);

                bufferSize *= 2;
                buffer = Memory_Allocate(bufferSize);
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

            information = (FILE_DIRECTORY_INFORMATION *)(((UINT_B)(buffer)) + i);

            if (Callback)
                Callback(Directory, information);

            if (information->NextEntryOffset != 0)
                i += information->NextEntryOffset;
            else
                break;
        }

        firstTime = FALSE;
    }

    Memory_Free(buffer);

    NtClose(directoryHandle);

    return status;
}


VOID Directory_AppendFileName(
    WCHAR* FileName,
    WCHAR* Path,
    WCHAR* Buffer
    )
{
    String_Copy(Buffer, Path);

    String_Concatenate(Buffer, L"\\");
    String_Concatenate(Buffer, FileName);
}
