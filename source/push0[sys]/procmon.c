#include "push0.h"
#include "ramdisk.h"


unsigned char g_bActivated = NULL;


typedef struct _MODULE
{
    WCHAR* FileName;
    UINT32 FileIndex;
} MODULE;


MODULE PmModules[] = {
  {L"d3d8.dll"},
  {L"d3d9.dll"},
  {L"d3d10.dll"},
  {L"d3d11.dll"}
};


/*
WCHAR* PmModuleNames[] = {L"d3d8.dll",     L"D3D8.DLL",
                       L"d3d9.dll",     L"D3D9.DLL",
                       L"d3d10.dll",    L"D3D10.DLL",
                       L"d3d11.dll",    L"D3D11.DLL"};*/


VOID
ProcessCallback(VOID *hParentId,
                VOID *hProcessId,
                BOOLEAN bCreate)
{
    if (bCreate)
    {
        DEVICE_EXTENSION *extension;

        //
        // Assign extension variable
        //
        extension = (DEVICE_EXTENSION*) g_DeviceObject->DeviceExtension;
        //
        // Assign current values into device extension.
        // User-mode apps will pick it up using DeviceIoControl calls.
        //
        extension->ProcessEvent.ProcessID = (UINT16) hProcessId;
        //
        // Signal the event thus the user-mode apps listening will be aware
        // that something interesting has happened.
        //

        KeSetEvent(ProcessEvent, 0, FALSE);

        KeClearEvent(ProcessEvent);
    }
}


VOID
ThreadCallback(VOID *processID,
               VOID *threadID,
               BOOLEAN create)
{
    DEVICE_EXTENSION *devExt;

    devExt = (DEVICE_EXTENSION*) g_DeviceObject->DeviceExtension;

    //devExt->ThreadEvent.threadOwner = (UINT16) processID;
    devExt->threadCallbackInfo.threadOwner = (UINT16) processID;
    devExt->threadCallbackInfo.threadID = (UINT16) threadID;
    devExt->threadCallbackInfo.create = create;


    KeSetEvent(ThreadEvent, 0, FALSE);

    KeClearEvent(ThreadEvent);
}


/*UINT32
GetFileIndex( WCHAR* FileName )
{
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrib;
    VOID *fileHandle;
    IO_STATUS_BLOCK isb;
    FILE_INTERNAL_INFORMATION fileInformation;
    NTSTATUS status;

    RtlInitUnicodeString(&fileName, FileName);

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = NULL;
    objAttrib.ObjectName = &fileName;
    objAttrib.Attributes = OBJ_CASE_INSENSITIVE;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    status = ZwCreateFile(
        &fileHandle,
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_READ,
        &objAttrib,
        &isb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if(!NT_SUCCESS(status))
    {
        return 0;
    }

    ZwQueryInformationFile(
        fileHandle,
        &isb,
        &fileInformation,
        sizeof(FILE_INTERNAL_INFORMATION),
        FileInternalInformation
        );

     ZwClose(fileHandle);

     return fileInformation.IndexNumber.u.LowPart;
}&/


/*VOID
ImageLoadCallback(
    UNICODE_STRING *imageName,
    VOID *processID,
    IMAGE_INFO *imageInfo
    )
{
    DEVICE_EXTENSION    *extension;
    INT32               i;
    PIMAGE_INFO_EX imageInfoEx = NULL;
    NTSTATUS status;
    FILE_INTERNAL_INFORMATION fileInformation;
    ULONG retLength;
    static UINT8 modules = sizeof(PmModules) / sizeof(PmModules[0]);

    extension = (DEVICE_EXTENSION*) g_DeviceObject->DeviceExtension;
    extension->imageEvent.processID = (UINT32) processID;

    // IMAGE_INFO_EX available on Vista, which will give us the backing file
    // object of the image section.
    if (imageInfo->ExtendedInfoPresent)
    {
        imageInfoEx = CONTAINING_RECORD(imageInfo, IMAGE_INFO_EX, ImageInfo);

        IoQueryFileInformation(
            imageInfoEx->FileObject,
            FileInternalInformation,
            sizeof(FILE_INTERNAL_INFORMATION),
            &fileInformation,
            &retLength
            );

        for (i = 0; i < modules; i++)
        {
            if (PmModules[i].FileIndex == fileInformation.IndexNumber.u.LowPart)
            {
                KeSetEvent(PushGpuAccelerationEvent, 0, FALSE);
                KeClearEvent(PushGpuAccelerationEvent);

                break;
            }

        }
    }
}*/


NTSTATUS
ToggleProcessMonitoring(PIRP Irp)
{
    BOOLEAN bActivate;

    bActivate = *(BOOLEAN *) Irp->AssociatedIrp.SystemBuffer;

    if (bActivate && !g_bActivated)
    {
        UINT8 i, modules;

        PsSetCreateProcessNotifyRoutine(ProcessCallback, 0);

        PsSetCreateThreadNotifyRoutine(ThreadCallback);

        // Populate file indexes
        /*modules = sizeof(PmModules) / sizeof(PmModules[0]);

        for (i = 0; i < modules; i++)
        {
            WCHAR filePath[260];
#ifdef _AMD64_
            wcscpy(filePath, L"\\DosDevices\\C:\\Windows\\SysWOW64\\");
#else
            wcscpy(filePath, L"\\DosDevices\\C:\\Windows\\System32\\");
#endif

            wcscat(filePath, PmModules[i].FileName);

            PmModules[i].FileIndex = GetFileIndex(
                                        filePath
                                        );
        }*/

        //PsSetLoadImageNotifyRoutine(ImageLoadCallback);
        FltFilterInstall( PushDriverObject );

        g_bActivated = 1;

        DbgPrint("[PUSH] Process Monitoring Activated\n");
    }
    else if (!bActivate && g_bActivated)
    {
        PsSetCreateProcessNotifyRoutine(ProcessCallback, 1);

        PsRemoveCreateThreadNotifyRoutine(ThreadCallback);

        //PsRemoveLoadImageNotifyRoutine(ImageLoadCallback);

        FltStopFiltering();

        g_bActivated = 0;

        DbgPrint("[PUSH] Process Monitoring De-Activated\n");
    }

    return STATUS_SUCCESS;
}
