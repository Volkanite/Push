#include <fltKernel.h>
#include "push0.h"

#define PUSH_RAMDISK_DEVICE_NAME L"\\Device\\PushRamDisk"

typedef struct _FLT_FILE_LIST_ENTRY FLT_FILE_LIST_ENTRY;
typedef struct _FLT_FILE_LIST_ENTRY
{
    WCHAR* Name;
    UINT16 NameLength;
    FLT_FILE_LIST_ENTRY* NextEntry;

} FLT_FILE_LIST_ENTRY;


// Forward declarations
FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreCreate(
    FLT_CALLBACK_DATA* Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext
    );

NTSTATUS FLTAPI FilterUnload(
    FLT_FILTER_UNLOAD_FLAGS Flags
    );

VOID AddToFileList(
    FLT_FILE_LIST_ENTRY *FileEntry
    );


PFLT_FILTER FltFilterData = NULL;
FLT_FILE_LIST_ENTRY* FltFileList = NULL;
BOOLEAN FltInitialized = FALSE;


VOID
FltFilterInstall( DRIVER_OBJECT* DriverObject )
{
    NTSTATUS status;
    FLT_REGISTRATION FilterRegistration;
    FLT_CONTEXT_REGISTRATION ContextReg[]=
    {
       {FLT_CONTEXT_END}
    };
    FLT_OPERATION_REGISTRATION OperationReg[]=
    {
        {IRP_MJ_CREATE, 0, FilterPreCreate, NULL, NULL},
        {IRP_MJ_OPERATION_END}
    };

    DbgPrint("[PUSH] => (PushFilterInstall)");

    memset(&FilterRegistration,0,sizeof(FilterRegistration));

    FilterRegistration.Size=sizeof(FilterRegistration);
    FilterRegistration.Version=FLT_REGISTRATION_VERSION;
    FilterRegistration.Flags=0;
    //we do not use any contexts yet

    FilterRegistration.ContextRegistration=ContextReg;
    FilterRegistration.OperationRegistration=OperationReg;
    FilterRegistration.FilterUnloadCallback = FilterUnload;

    status = FltRegisterFilter(
                    DriverObject,
                    &FilterRegistration,
                    &FltFilterData
                    );

    if (NT_SUCCESS(status))
    {
        FltStartFiltering(FltFilterData);

        FltInitialized = TRUE;
    }

    DbgPrint("[PUSH] <= (PushFilterInstall)");
}


VOID
FltStopFiltering()
{
    FLT_FILE_LIST_ENTRY *file = FltFileList;

    DbgPrint("[PUSH] => (FltStopFiltering)");

    FltUnregisterFilter(FltFilterData);

    FltFileList = NULL;

    while (file != 0)
    {
        ExFreePool(file->Name);

        file = file->NextEntry;
    }

    DbgPrint("[PUSH] <= (FltStopFiltering)");
}


VOID
FltQueueFile( WCHAR* File )
{
    FLT_FILE_LIST_ENTRY fileEntry;

    fileEntry.Name = File;

    AddToFileList(&fileEntry);
}


NTSTATUS
FLTAPI
FilterUnload(
    FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    DbgPrint("[PUSH] => (PushFilterUnload)");
    DbgPrint("[PUSH] <= (PushFilterUnload)");

    return STATUS_SUCCESS;
}


FLT_PREOP_CALLBACK_STATUS
FLTAPI
FilterPreCreate(
    FLT_CALLBACK_DATA* Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext
    )
{
    NTSTATUS status;
    FLT_FILE_NAME_INFORMATION *nameInformation;
    UNICODE_STRING *fullPath;

    status = FltGetFileNameInformation(
                Data,
                FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                &nameInformation
                );

    if (status == STATUS_SUCCESS)
    {
        status = FltParseFileNameInformation(nameInformation);

        if (status == STATUS_SUCCESS)
        {
            FLT_FILE_LIST_ENTRY *file = FltFileList;

            fullPath = &nameInformation->Name;

            if ((Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_EXECUTE) //Execute
                && (nameInformation->FinalComponent.Length == 16 //d3d8.dll/d3d9.dll
                || nameInformation->FinalComponent.Length == 18)) //d3d10.dll/d3d11.dll
            {
                if (wcsncmp(L"d3d8.dll", nameInformation->FinalComponent.Buffer, 8) == 0
                    || wcsncmp(L"d3d9.dll", nameInformation->FinalComponent.Buffer, 8) == 0
                    || wcsncmp(L"d3d10.dll", nameInformation->FinalComponent.Buffer, 9) == 0
                    || wcsncmp(L"d3d11.dll", nameInformation->FinalComponent.Buffer, 9) == 0)
                {
                    UINT32 processId;
                    DEVICE_EXTENSION *deviceExtension;

                    processId = FltGetRequestorProcessId(Data);

                    deviceExtension = g_DeviceObject->DeviceExtension;
                    deviceExtension->imageEvent.processID = processId;

                    KeSetEvent(PushGpuAccelerationEvent, 0, FALSE);
                    KeClearEvent(PushGpuAccelerationEvent);

                    DbgPrint("[PUSH] %u loaded %wZ\n", processId, fullPath);
                }
            }

            while (file != 0)
            {
                if (fullPath->Length == file->NameLength
                    && wcsncmp(file->Name, fullPath->Buffer, file->NameLength / sizeof(WCHAR)) == 0)
                {
                    UNICODE_STRING newName;
                    UNICODE_STRING redirect;
                    UNICODE_STRING *currentName;
                    UINT16 newNameSize;
                    WCHAR *buffer;

                    //DbgPrint("[PUSH] %ws matches %wZ", file->Name, fullPath);

                    RtlInitUnicodeString(&newName, PUSH_RAMDISK_DEVICE_NAME L"\\");

                    // Calculate the size of the name
                    newNameSize = newName.Length + nameInformation->FinalComponent.Length;

                    redirect.Length = newName.Length;
                    redirect.MaximumLength = newNameSize;
                    redirect.Buffer = ExAllocatePool(NonPagedPool, newNameSize);

                    // Copy ramdisk device name into buffer
                    RtlCopyMemory(redirect.Buffer, newName.Buffer, newName.Length);

                    // Append original file name
                    //RtlAppendUnicodeStringToString(&redirect, &nameInformation->FinalComponent);
                    buffer = &redirect.Buffer[redirect.Length / sizeof(WCHAR)];

                    RtlMoveMemory(
                        buffer,
                        nameInformation->FinalComponent.Buffer,
                        nameInformation->FinalComponent.Length
                        );

                    redirect.Length += nameInformation->FinalComponent.Length;
                    currentName = &Data->Iopb->TargetFileObject->FileName;

                    // Throw away the current file name
                    ExFreePool(currentName->Buffer);

                    currentName->Length = 0;
                    currentName->MaximumLength = redirect.MaximumLength;
                    currentName->Buffer = ExAllocatePool(NonPagedPool, redirect.MaximumLength);

                    // Copy new path to buffer
                    RtlCopyUnicodeString(currentName, &redirect);

                    // Be nice and give back what was given to us
                    ExFreePool(redirect.Buffer);

                    Data->IoStatus.Information = IO_REPARSE;
                    Data->IoStatus.Status = STATUS_REPARSE;
                    Data->Iopb->TargetFileObject->RelatedFileObject = NULL;

                    FltSetCallbackDataDirty(Data);

                    return FLT_PREOP_COMPLETE;
                }

                file = file->NextEntry;
            }
        }
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


VOID
AddToFileList( FLT_FILE_LIST_ENTRY *FileEntry )
{
    FLT_FILE_LIST_ENTRY *fileListEntry;
    WCHAR *name;
    UINT16 nameLength;

    nameLength = wcslen(FileEntry->Name) * sizeof(WCHAR);

    name = ExAllocatePool(
            NonPagedPool,
            nameLength + (1 * sizeof(WCHAR))
            );

    wcscpy(name, FileEntry->Name);

    if (FltFileList == 0)
    {
        FltFileList = ExAllocatePool(
            NonPagedPool,
            sizeof(FLT_FILE_LIST_ENTRY)
            );

        ((FLT_FILE_LIST_ENTRY*)FltFileList)->NextEntry = NULL;
        ((FLT_FILE_LIST_ENTRY*)FltFileList)->Name = name;
        ((FLT_FILE_LIST_ENTRY*)FltFileList)->NameLength = nameLength;

        return;
    }

    fileListEntry = (FLT_FILE_LIST_ENTRY*) FltFileList;

    while (fileListEntry->NextEntry != 0)
    {
        fileListEntry = fileListEntry->NextEntry;
    }

    fileListEntry->NextEntry = (FLT_FILE_LIST_ENTRY *) ExAllocatePool(
        NonPagedPool,
        sizeof(FLT_FILE_LIST_ENTRY)
        );

    fileListEntry = fileListEntry->NextEntry;
    fileListEntry->Name = name;
    fileListEntry->NameLength = nameLength;
    fileListEntry->NextEntry = 0;
}
