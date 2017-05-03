#include <sl.h>
#include <slresource.h>


BOOLEAN Resource_Extract( WCHAR* ResourceName, WCHAR* OutputPath )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    HANDLE hResInfo;
    HANDLE hResData;
    VOID* pDeskbandBinData;
    DWORD dwDeskbandBinSize;

    hResInfo = FindResourceW(NULL, ResourceName, ResourceName);

    if(!hResInfo)
    {
        return FALSE;
    }

    hResData = LoadResource(NULL, hResInfo);

    if(!hResData)
    {
        return FALSE;
    }

    pDeskbandBinData = LockResource(hResData);

    if(!pDeskbandBinData)
    {
        return FALSE;
    }

    dwDeskbandBinSize = SizeofResource(NULL, hResInfo);

    if(!dwDeskbandBinSize)
    {
        return FALSE;
    }

    status = File_Create(
        &fileHandle,
        OutputPath,
        FILE_READ_ATTRIBUTES | GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_CREATE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    status = NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        pDeskbandBinData,
        dwDeskbandBinSize,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    NtClose(fileHandle);

    return TRUE;
}
