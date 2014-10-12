#include "push0.h"
#include "filter.h"
#include <slpci.h>


//
// Allocation bitmap with currently cnfigured device numbers.
//
//volatile ULONGLONG DeviceList = 0;

//
// An array of boolean values for each drive letter where TRUE means a
// drive letter disallowed for use by ImDisk devices.
//
BOOLEAN DisallowedDriveLetters[L'Z'-L'A'+1] = { FALSE };


DISK_GEOMETRY media_table[] = {
  // 3.5" UHD
  { { 963 }, F3_120M_512,  8, 32, 512 },
  { { 262 }, F3_120M_512, 32, 56, 512 },
  // 3.5"
  { {  80 }, F3_2Pt88_512, 2, 36, 512 },
  { {  82 }, F3_1Pt44_512, 2, 21, 512 },
  { {  80 }, F3_1Pt44_512, 2, 21, 512 },
  { {  80 }, F3_1Pt44_512, 2, 18, 512 },
  { {  82 }, F3_720_512,   2, 10, 512 },
  { {  80 }, F3_720_512,   2,  9, 512 },
  // 5.25"
  { {  80 }, F5_1Pt2_512,  2, 15, 512 },
  { {  40 }, F5_640_512,   2, 18, 512 },
  { {  40 }, F5_360_512,   2,  9, 512 },
  { {  40 }, F5_320_512,   2,  8, 512 },
  { {  40 }, F5_180_512,   1,  9, 512 },
  { {  40 }, F5_160_512,   1,  8, 512 }
};


PDRIVER_OBJECT  PushDriverObject     = NULL;
PDEVICE_OBJECT  g_DeviceObject      = NULL;
PDEVICE_OBJECT  g_pDiskDeviceObject = NULL;
PKEVENT         ProcessEvent        = NULL;
PKEVENT         ThreadEvent         = NULL;
PKEVENT         PushGpuAccelerationEvent          = NULL;


NTSTATUS
DriverEntry( DRIVER_OBJECT *DriverObject, UNICODE_STRING *RegistryPath )
{
    VOID *processEventHandle = 0;
    VOID *threadEventHandle = 0;
    VOID *imageEventHandle = 0;
    NTSTATUS status;
    UNICODE_STRING DeviceNameU;
    UNICODE_STRING DeviceLinkU;
    UNICODE_STRING processEventName;
    UNICODE_STRING threadEventName;
    UNICODE_STRING imageEventName;

    DbgPrint("[PUSH] => (DriverEntry)\n");

    MmPageEntireDriver((PVOID)(ULONG_PTR) DriverEntry);

    MaxDevices = IMDISK_DEFAULT_MAX_DEVICES;

    lstMapInfo.Next=NULL;

    PushDriverObject = DriverObject;

    RtlInitUnicodeString(&DeviceNameU,  PUSH_DEVICE_BASE_NAME);
    RtlInitUnicodeString(&DeviceLinkU,  PUSH_SYMLINK_NAME);
    RtlInitUnicodeString(&processEventName, L"\\BaseNamedObjects\\" PUSH_PROCESS_EVENT_NAME);
    RtlInitUnicodeString(&threadEventName,  L"\\BaseNamedObjects\\" PUSH_THREAD_EVENT_NAME);
    RtlInitUnicodeString(&imageEventName,   L"\\BaseNamedObjects\\" PUSH_IMAGE_EVENT_NAME);

    //Create a device object
    status = IoCreateDevice(DriverObject,               //IN: Driver Object
                              sizeof(DEVICE_EXTENSION), //IN: Device Extension Size
                              &DeviceNameU,             //IN: Device Name
                              FILE_DEVICE_PUSH,         //IN: Device Type
                              0,                        //IN: Device Characteristics
                              FALSE,                    //IN: Exclusive
                              &g_DeviceObject);     //OUT:Created Device Object

    // The control device gets a device_number of -1 to make it easily
    // distinguishable from the actual created devices.
    //((PDEVICE_EXTENSION) g_DeviceObject->DeviceExtension)->device_number =
    //(ULONG) -1;

    //Dispatch functions
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_READ]            = ImDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = ImDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = DispatchIoCtl;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = ImDiskDispatchPnP;

    DriverObject->DriverUnload = PushUnload;

    //Create a symbolic link
    IoCreateSymbolicLink(&DeviceLinkU, &DeviceNameU);

    ProcessEvent = IoCreateNotificationEvent(&processEventName, &processEventHandle);
    ThreadEvent = IoCreateNotificationEvent(&threadEventName, &threadEventHandle);
    PushGpuAccelerationEvent = IoCreateNotificationEvent(&imageEventName, &imageEventHandle);

    KeClearEvent(ProcessEvent);
    KeClearEvent(ThreadEvent);
    KeClearEvent(PushGpuAccelerationEvent);

    DbgPrint("[PUSH] <= (DriverEntry)\n");

    return status;
}


/*++
IRP_MJ_CREATE dispatch routine
--*/
NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    Irp->IoStatus.Status        = STATUS_SUCCESS;
    Irp->IoStatus.Information   = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


/*++
IRP_MJ_CLOSE dispatch routine
--*/
NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    Irp->IoStatus.Status        = STATUS_SUCCESS;
    Irp->IoStatus.Information   = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS ReadMsr(
    void *lpInBuffer,
    ULONG nInBufferSize,
    void *lpOutBuffer,
    ULONG nOutBufferSize,
    ULONG *lpBytesReturned
    );
NTSTATUS
    ReadPciConfig(  void    *lpInBuffer,
    ULONG   nInBufferSize,
    void    *lpOutBuffer,
    ULONG   nOutBufferSize,
    ULONG   *lpBytesReturned
    );
/*NTSTATUS ReadGpuRegister(
    void            *lpInBuffer,
    unsigned long   nInBufferSize,
    void            *lpOutBuffer,
    unsigned long   nOutBufferSize,
    unsigned long   *lpBytesReturned
    );*/
NTSTATUS SetCacheName(void *lpInBuffer, ULONG nInBufferSize, void *lpOutBuffer, ULONG nOutBufferSize, ULONG *lpBytesReturned);
NTSTATUS
    MapPhysicalMemory(
    VOID* InputBuffer,
    ULONG InputBufferLength,
    ULONG OutputBufferLength,
    ULONG* BytesReturned
    );
/*++
IRP_MJ_DEVICE_CONTROL dispatch routine
--*/
NTSTATUS DispatchIoCtl(IN PDEVICE_OBJECT fdo, IN PIRP irp)
{
    PDEVICE_EXTENSION device_extension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;

    //Init to default settings
    irp->IoStatus.Status=STATUS_SUCCESS;
    irp->IoStatus.Information=0;

    irpStack=IoGetCurrentIrpStackLocation(irp);

    switch (irpStack->MajorFunction)
    {
    case IRP_MJ_DEVICE_CONTROL:

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_PUSH_READ_MSR:
            {
                irp->IoStatus.Status = ReadMsr(
                    irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.InputBufferLength,
                    irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                    (ULONG*)&irp->IoStatus.Information
                    );

                ntStatus=irp->IoStatus.Status;

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return ntStatus;

            } break;

        case IOCTL_PUSH_READ_PCI_CONFIG:
            {
                /*irp->IoStatus.Status = ReadPciConfig(
                    irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.InputBufferLength,
                    irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                    (ULONG*)&irp->IoStatus.Information
                    );*/

                irp->IoStatus.Status = SlReadPciConfig(
                                        irp->AssociatedIrp.SystemBuffer,
                                        irpStack->Parameters.DeviceIoControl.InputBufferLength,
                                        irp->AssociatedIrp.SystemBuffer,
                                        irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                                        (ULONG*)&irp->IoStatus.Information
                                        );

                ntStatus=irp->IoStatus.Status;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ntStatus;

            } break;

        /*case IOCTL_PUSH_READ_GPU_REGISTER:
            {
                irp->IoStatus.Status = ReadGpuRegister(
                    irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.InputBufferLength,
                    irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                    (ULONG*)&irp->IoStatus.Information
                    );

                ntStatus=irp->IoStatus.Status;

                IoCompleteRequest(irp, IO_NO_INCREMENT);
                return ntStatus;

            } break;*/

        case IOCTL_PUSH_SET_CACHE_NAME:
            {
                irp->IoStatus.Status = SetCacheName(
                    irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.InputBufferLength,
                    irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                    (ULONG*)&irp->IoStatus.Information
                    );

                ntStatus=irp->IoStatus.Status;

                IoCompleteRequest(irp, IO_NO_INCREMENT);
                return ntStatus;

            } break;

        case IOCTL_PUSH_TOGGLE_MONITORING:
            {
                ntStatus = ToggleProcessMonitoring(irp);

                irp->IoStatus.Status = ntStatus;

                irp->IoStatus.Information =
                irpStack->Parameters.DeviceIoControl.OutputBufferLength;

                IoCompleteRequest(irp, IO_NO_INCREMENT);

                return ntStatus;

            } break;

            case IOCTL_PUSH_GET_PROC_INFO:
            {
                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
                   sizeof(PROCESS_CALLBACK_INFO))
                {
                    PPROCESS_CALLBACK_INFO pProcCallbackInfo;
                    PDEVICE_EXTENSION     extension = g_DeviceObject->DeviceExtension;


                    pProcCallbackInfo               = irp->AssociatedIrp.SystemBuffer;
                    pProcCallbackInfo->hProcessID   = extension->ProcessEvent.ProcessID;

                    ntStatus = STATUS_SUCCESS;
                }

                irp->IoStatus.Status = ntStatus;

                irp->IoStatus.Information =
                irpStack->Parameters.DeviceIoControl.OutputBufferLength;

                IoCompleteRequest(irp, IO_NO_INCREMENT);

                return ntStatus;

            } break;

            case IOCTL_PUSH_GET_IMAGE_INFO:
                {
                    IMAGE_CALLBACK_INFO *imageInfo;
                    PDEVICE_EXTENSION extension = g_DeviceObject->DeviceExtension;


                    imageInfo               = irp->AssociatedIrp.SystemBuffer;
                    imageInfo->processID    = extension->imageEvent.processID;

                    /*wcscpy(imageInfo->imageName,
                           extension->imageEvent.imageName);*/

                    ntStatus = STATUS_SUCCESS;

                    irp->IoStatus.Status = ntStatus;

                    irp->IoStatus.Information =
                    irpStack->Parameters.DeviceIoControl.OutputBufferLength;

                    IoCompleteRequest(irp, IO_NO_INCREMENT);

                    return ntStatus;

                } break;

            case IOCTL_PUSH_GET_THREAD_INFO:
                {
                    THREAD_CALLBACK_INFO *threadInfo;
                    DEVICE_EXTENSION *extension;

                    extension = (DEVICE_EXTENSION*) g_DeviceObject->DeviceExtension;

                    threadInfo = (THREAD_CALLBACK_INFO*) irp->AssociatedIrp.SystemBuffer;

                    //threadInfo->threadOwner = extension->ThreadEvent.threadOwner;
                    threadInfo->threadOwner = extension->threadCallbackInfo.threadOwner;
                    threadInfo->threadID = extension->threadCallbackInfo.threadID;
                    threadInfo->create = extension->threadCallbackInfo.create;

                    ntStatus = STATUS_SUCCESS;

                    irp->IoStatus.Status        = ntStatus;
                    irp->IoStatus.Information   = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

                    IoCompleteRequest(irp, IO_NO_INCREMENT);

                    return ntStatus;

                } break;

            case IOCTL_PUSH_MAP_PHYSICAL_MEMORY:
                {
                    irp->IoStatus.Status = MapPhysicalMemory(
                                            irp->AssociatedIrp.SystemBuffer,
                                            irpStack->Parameters.DeviceIoControl.InputBufferLength,
                                            irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                                            (ULONG*)&irp->IoStatus.Information
                                            );

                    IoCompleteRequest(irp, IO_NO_INCREMENT);

                    return irp->IoStatus.Status;

                } break;

            case IOCTL_PUSH_QUEUE_FILE:
                {
                    DbgPrint("[PUSH] File request: %ws", irp->AssociatedIrp.SystemBuffer);

                    FltQueueFile(irp->AssociatedIrp.SystemBuffer);

                    irp->IoStatus.Status = STATUS_SUCCESS;

                    /*if (!FltInitialized)
                        FltFilterInstall(PushDriverObject);*/

                    IoCompleteRequest(irp, IO_NO_INCREMENT);

                    return irp->IoStatus.Status;

                } break;
        }

        break;
    }

    ASSERT(fdo != NULL);
    ASSERT(irp != NULL);

    device_extension = (PDEVICE_EXTENSION) fdo->DeviceExtension;

    if (KeReadStateEvent(&device_extension->terminate_thread) != 0)
    {
        DbgPrint("ImDisk: IOCTL attempt on device that is being removed.\n");
           //device_extension->device_number));

        ntStatus = STATUS_NO_MEDIA_IN_DEVICE;

        irp->IoStatus.Status = ntStatus;
        irp->IoStatus.Information = 0;

        IoCompleteRequest(irp, IO_NO_INCREMENT);

        return ntStatus;
    }

  // The control device can only receive version queries, enumeration queries
  // or device create requests.
  if (fdo == g_DeviceObject)
    switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
      {
      case IOCTL_PUSH_QUERY_VERSION:
      case IOCTL_PUSH_CREATE_RAMDISK:
      case IOCTL_PUSH_QUERY_DRIVER:
    break;

      default:
    KdPrint(("ImDisk: Invalid IOCTL %#x for control device.\n",
         irpStack->Parameters.DeviceIoControl.IoControlCode));

    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    irp->IoStatus.Status = ntStatus;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return ntStatus;
      }
  else
    switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
      {
    // Invalid IOCTL codes for this driver's disk devices.
      case IOCTL_PUSH_CREATE_RAMDISK:
    KdPrint(("ImDisk: Invalid IOCTL %#x for disk device.\n",
         irpStack->Parameters.DeviceIoControl.IoControlCode));

    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    irp->IoStatus.Status = ntStatus;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return ntStatus;
      }

  switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_PUSH_CREATE_RAMDISK:
      {
    RAMDISK_CREATE_DATA* create_data;

    KdPrint(("ImDisk: IOCTL_IMDISK_CREATE_DEVICE for device.\n"));
         //device_extension->device_number));

    // This IOCTL requires work that must be done at IRQL < DISPATCH_LEVEL
    // but the control device has no worker thread (does not handle any
    // other I/O) so therefore everything is done directly here. Therefore
    // this IRQL check is necessary.
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
      {
        ntStatus = STATUS_ACCESS_DENIED;
        irp->IoStatus.Information = 0;
        break;
      }

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(RAMDISK_CREATE_DATA) /*- sizeof(*create_data->FileName)*/)
      {
        KdPrint(("ImDisk: Invalid input buffer size (1). "
             "Got: %u Expected at least: %u.\n",
             irpStack->Parameters.DeviceIoControl.InputBufferLength,
             sizeof(IMDISK_CREATE_DATA) -
             sizeof(*create_data->FileName)));

        ntStatus = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Information = 0;
        break;
      }

    create_data = (RAMDISK_CREATE_DATA*) irp->AssociatedIrp.SystemBuffer;

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(RAMDISK_CREATE_DATA) /*+
        create_data->FileNameLength -
        sizeof(*create_data->FileName)*/)
      {
        KdPrint(("ImDisk: Invalid input buffer size (2). "
             "Got: %u Expected at least: %u.\n",
             irpStack->Parameters.DeviceIoControl.InputBufferLength,
             sizeof(IMDISK_CREATE_DATA) +
             create_data->FileNameLength -
             sizeof(*create_data->FileName)));

        ntStatus = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Information = 0;
        break;
      }

    ntStatus = ImDiskAddVirtualDisk(fdo->DriverObject,
                      (RAMDISK_CREATE_DATA*)
                      irp->AssociatedIrp.SystemBuffer,
                      irp->Tail.Overlay.Thread);

    if (NT_SUCCESS(ntStatus) &&
        (irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
         irpStack->Parameters.DeviceIoControl.InputBufferLength))
      irp->IoStatus.Information =
        irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    else
      irp->IoStatus.Information = 0;
    break;
      }

    case IOCTL_DISK_EJECT_MEDIA:
    case IOCTL_STORAGE_EJECT_MEDIA:
      KdPrint(("ImDisk: IOCTL_DISK/STORAGE_EJECT_MEDIA for device.\n"));
           //device_extension->device_number));

      if (device_extension->special_file_count > 0)
    {
      irp->IoStatus.Information = 0;
      ntStatus = STATUS_ACCESS_DENIED;
    }
      else
    {
      ImDiskRemoveVirtualDisk(fdo);

      irp->IoStatus.Information = 0;
      ntStatus = STATUS_SUCCESS;
    }

      break;

    case IOCTL_PUSH_QUERY_RAMDISK:
        {
            RAMDISK_CREATE_DATA *create_data;

            KdPrint(("[PUSH] IOCTL_IMDISK_QUERY_DEVICE for device.\n"));

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(RAMDISK_CREATE_DATA) /*+
                device_extension->file_name.Length +
                sizeof(*create_data->FileName)*/)
            {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                irp->IoStatus.Information = 0;
                break;
            }

            create_data = (RAMDISK_CREATE_DATA*) irp->AssociatedIrp.SystemBuffer;

            //create_data->DeviceNumber = device_extension->device_number;
            create_data->DiskGeometry = device_extension->disk_geometry;

            //create_data->Flags = 0;
            /*if (device_extension->read_only)
              create_data->Flags |= IMDISK_OPTION_RO;*/

            /*if (fdo->Characteristics & FILE_REMOVABLE_MEDIA)
              create_data->Flags |= IMDISK_OPTION_REMOVABLE;*/

            /*if (fdo->DeviceType == FILE_DEVICE_CD_ROM)
              create_data->Flags |= IMDISK_DEVICE_TYPE_CD | IMDISK_OPTION_RO;
            else if (fdo->Characteristics & FILE_FLOPPY_DISKETTE)
              create_data->Flags |= IMDISK_DEVICE_TYPE_FD;
            else
              create_data->Flags |= IMDISK_DEVICE_TYPE_HD;*/

            /*if (device_extension->vm_disk)
              create_data->Flags |= IMDISK_TYPE_VM;
            else if (device_extension->use_proxy)
              create_data->Flags |= IMDISK_TYPE_PROXY;
            else
              create_data->Flags |= IMDISK_TYPE_FILE;*/

            /*if (device_extension->image_modified)
              create_data->Flags |= IMDISK_IMAGE_MODIFIED;*/

            /*if (device_extension->use_set_zero_data)
              create_data->Flags |= IMDISK_OPTION_SPARSE_FILE;*/

            //create_data->ImageOffset = device_extension->image_offset;

            create_data->DriveLetter = device_extension->drive_letter;

            //create_data->FileNameLength = device_extension->file_name.Length;

            /*if (device_extension->file_name.Length > 0)
              RtlCopyMemory(create_data->FileName,
                    device_extension->file_name.Buffer,
                    device_extension->file_name.Length);*/

            ntStatus = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(RAMDISK_CREATE_DATA)/* +
              create_data->FileNameLength -
              sizeof(*create_data->FileName)*/;

            break;
      }

    case IOCTL_DISK_CHECK_VERIFY:
    //case IOCTL_CDROM_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY2:
      {
    KdPrint(("ImDisk: IOCTL_DISK/CDROM/STORAGE_CHECK_VERIFY/2 for "
         "device.\n"/*, device_extension->device_number*/));

    if (device_extension->vm_disk)
      {
        KdPrint(("ImDisk: Faked verify ok on vm device.\n"/*,
             device_extension->device_number*/));

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(ULONG))
          irp->IoStatus.Information = 0;
        else
          {
        *(PULONG) irp->AssociatedIrp.SystemBuffer =
          device_extension->media_change_count;

        irp->IoStatus.Information = sizeof(ULONG);
          }

        ntStatus = STATUS_SUCCESS;
      }
    else
      ntStatus = STATUS_PENDING;

    break;
      }

    case IOCTL_PUSH_QUERY_VERSION:
      {
    KdPrint(("ImDisk: IOCTL_IMDISK_QUERY_VERSION for device.\n"/*,
         device_extension->device_number*/));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(ULONG))
      ntStatus = STATUS_INVALID_PARAMETER;
    else
      {
        *(PULONG) irp->AssociatedIrp.SystemBuffer = IMDISK_DRIVER_VERSION;
        irp->IoStatus.Information = sizeof(ULONG);
        ntStatus = STATUS_SUCCESS;
      }

    break;
      }

    case IOCTL_DISK_FORMAT_TRACKS:
    case IOCTL_DISK_FORMAT_TRACKS_EX:
      //    Only several checks are done here
      //    Actual operation is done by the device thread
      {
    PFORMAT_PARAMETERS param;
    PDISK_GEOMETRY geometry;

    KdPrint(("ImDisk: IOCTL_DISK_FORMAT_TRACKS for device.\n"/*,
         device_extension->device_number*/));

    /*
    if (~fdo->Characteristics & FILE_FLOPPY_DISKETTE)
      {
        irp->IoStatus.Information = 0;
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
      }
    */

    //  Media is writable?

    if (device_extension->read_only)
      {
        KdPrint(("ImDisk: Attempt to format write-protected image.\n"));

        irp->IoStatus.Information = 0;
        ntStatus = STATUS_MEDIA_WRITE_PROTECTED;
        break;
      }

    //  Check input parameter size

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FORMAT_PARAMETERS))
      {
        irp->IoStatus.Information = 0;
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
      }

    //  Input parameter sanity check

    param = (PFORMAT_PARAMETERS) irp->AssociatedIrp.SystemBuffer;
    geometry = ExAllocatePoolWithTag(NonPagedPool,
                     sizeof(DISK_GEOMETRY),
                     POOL_TAG);
    if (geometry == NULL)
      {
        irp->IoStatus.Information = 0;
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        break;
      }

    RtlCopyMemory(geometry, &device_extension->disk_geometry,
              sizeof(DISK_GEOMETRY));

    geometry->Cylinders.QuadPart /= geometry->TracksPerCylinder;
    geometry->Cylinders.QuadPart /= geometry->SectorsPerTrack;
    geometry->Cylinders.QuadPart /= geometry->BytesPerSector;

    if ((param->StartHeadNumber > geometry->TracksPerCylinder - 1) ||
        (param->EndHeadNumber   > geometry->TracksPerCylinder - 1) ||
        ((LONGLONG)param->StartCylinderNumber >
         geometry->Cylinders.QuadPart) ||
        ((LONGLONG)param->EndCylinderNumber >
         geometry->Cylinders.QuadPart) ||
        (param->EndCylinderNumber   < param->StartCylinderNumber))
      {
        ExFreePoolWithTag(geometry, POOL_TAG);
        irp->IoStatus.Information = 0;
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
      }

    if ((param->StartCylinderNumber * geometry->TracksPerCylinder *
         geometry->BytesPerSector * geometry->SectorsPerTrack +
         param->StartHeadNumber * geometry->BytesPerSector *
         geometry->SectorsPerTrack >=
         device_extension->disk_geometry.Cylinders.QuadPart) |
        (param->EndCylinderNumber * geometry->TracksPerCylinder *
         geometry->BytesPerSector * geometry->SectorsPerTrack +
         param->EndHeadNumber * geometry->BytesPerSector *
         geometry->SectorsPerTrack >=
         device_extension->disk_geometry.Cylinders.QuadPart))
      {
        ExFreePoolWithTag(geometry, POOL_TAG);
        irp->IoStatus.Information = 0;
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
      }

    //  If this is an EX request then make a couple of extra checks

    if (irpStack->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_DISK_FORMAT_TRACKS_EX)
      {
        PFORMAT_EX_PARAMETERS exparam;
        ULONG paramsize;

        KdPrint(("ImDisk: IOCTL_DISK_FORMAT_TRACKS_EX for device.\n"/*,
             device_extension->device_number*/));

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FORMAT_EX_PARAMETERS))
          {
        ExFreePoolWithTag(geometry, POOL_TAG);
        irp->IoStatus.Information = 0;
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
          }

        exparam = (PFORMAT_EX_PARAMETERS)irp->AssociatedIrp.SystemBuffer;

        paramsize = sizeof(FORMAT_EX_PARAMETERS)
          + exparam->SectorsPerTrack * sizeof(USHORT)
          - sizeof(USHORT);

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        paramsize ||
        exparam->FormatGapLength > geometry->SectorsPerTrack ||
        exparam->SectorsPerTrack != geometry->SectorsPerTrack)
          {
        ExFreePoolWithTag(geometry, POOL_TAG);
        irp->IoStatus.Information = 0;
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
          }
      }

    ExFreePoolWithTag(geometry, POOL_TAG);
    ntStatus = STATUS_PENDING;
    break;
      }

    case IOCTL_DISK_GROW_PARTITION:
      {
    PDISK_GROW_PARTITION grow_partition;

    KdPrint(("ImDisk: IOCTL_DISK_GROW_PARTITION for device.\n"/*,
         device_extension->device_number*/));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength !=
        sizeof(DISK_GROW_PARTITION))
      {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        irp->IoStatus.Information = 0;
        break;
      }

    if (device_extension->read_only)
      {
        ntStatus = STATUS_MEDIA_WRITE_PROTECTED;
        irp->IoStatus.Information = 0;
        break;
      }

    grow_partition = (PDISK_GROW_PARTITION)
      irp->AssociatedIrp.SystemBuffer;

    // Check so we don't get a smaller disk with these parameters
    if ((grow_partition->PartitionNumber != 1) |
        (device_extension->disk_geometry.Cylinders.QuadPart +
         grow_partition->BytesToGrow.QuadPart <
         device_extension->disk_geometry.Cylinders.QuadPart))
      {
        ntStatus = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Information = 0;
        break;
      }

    ntStatus = STATUS_PENDING;
    break;
      }

    case IOCTL_DISK_UPDATE_PROPERTIES:
      {
    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    break;
      }

    case IOCTL_DISK_GET_MEDIA_TYPES:
    case IOCTL_STORAGE_GET_MEDIA_TYPES:
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    //case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
    case IOCTL_DISK_UPDATE_DRIVE_SIZE:
      {
    PDISK_GEOMETRY geometry;

    KdPrint(("ImDisk: IOCTL_DISK/STORAGE_GET_MEDIA_TYPES/DRIVE_GEOMETRY "
         "for device.\n"/*, device_extension->device_number*/));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(device_extension->disk_geometry))
      {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        irp->IoStatus.Information = 0;
        break;
      }

    geometry = (PDISK_GEOMETRY) irp->AssociatedIrp.SystemBuffer;
    *geometry = device_extension->disk_geometry;
    geometry->Cylinders.QuadPart /= geometry->TracksPerCylinder;
    geometry->Cylinders.QuadPart /= geometry->SectorsPerTrack;
    geometry->Cylinders.QuadPart /= geometry->BytesPerSector;

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
    break;
      }

    case IOCTL_DISK_GET_LENGTH_INFO:
      {
    KdPrint(("ImDisk: IOCTL_DISK_GET_LENGTH_INFO for device %i.\n"/*,
         device_extension->device_number*/));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(GET_LENGTH_INFORMATION))
      {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        irp->IoStatus.Information = 0;
        break;
      }

    ((PGET_LENGTH_INFORMATION) irp->AssociatedIrp.SystemBuffer)->
      Length.QuadPart =
      device_extension->disk_geometry.Cylinders.QuadPart;

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);

    break;
      }

    case IOCTL_DISK_GET_PARTITION_INFO:
      {
    PPARTITION_INFORMATION partition_information;

    KdPrint(("ImDisk: IOCTL_DISK_GET_PARTITION_INFO for device.\n"/*,
         device_extension->device_number*/));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(PARTITION_INFORMATION))
      {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        irp->IoStatus.Information = 0;
        break;
      }

    partition_information =
      (PPARTITION_INFORMATION) irp->AssociatedIrp.SystemBuffer;

    partition_information->StartingOffset.QuadPart =
      (LONGLONG) device_extension->disk_geometry.BytesPerSector *
      device_extension->disk_geometry.SectorsPerTrack;
    partition_information->PartitionLength =
      device_extension->disk_geometry.Cylinders;
    partition_information->HiddenSectors = 1;
    partition_information->PartitionNumber = 1;
    partition_information->PartitionType = PARTITION_HUGE;
    partition_information->BootIndicator = FALSE;
    partition_information->RecognizedPartition = FALSE;
    partition_information->RewritePartition = FALSE;

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);

    break;
      }

    case IOCTL_DISK_GET_PARTITION_INFO_EX:
      {
    PPARTITION_INFORMATION_EX partition_information_ex;

    KdPrint(("ImDisk: IOCTL_DISK_GET_PARTITION_INFO_EX for device.\n"/*,
         device_extension->device_number*/));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(PARTITION_INFORMATION_EX))
      {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        irp->IoStatus.Information = 0;
        break;
      }

    partition_information_ex =
      (PPARTITION_INFORMATION_EX) irp->AssociatedIrp.SystemBuffer;

    partition_information_ex->PartitionStyle = PARTITION_STYLE_MBR;
    partition_information_ex->StartingOffset.QuadPart =
      (LONGLONG) device_extension->disk_geometry.BytesPerSector *
      device_extension->disk_geometry.SectorsPerTrack;
    partition_information_ex->PartitionLength =
      device_extension->disk_geometry.Cylinders;
    partition_information_ex->PartitionNumber = 1;
    partition_information_ex->RewritePartition = FALSE;
    partition_information_ex->Mbr.PartitionType = PARTITION_HUGE;
    partition_information_ex->Mbr.BootIndicator = FALSE;
    partition_information_ex->Mbr.RecognizedPartition = FALSE;
    partition_information_ex->Mbr.HiddenSectors = 1;

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);

    break;
      }

    case IOCTL_DISK_IS_WRITABLE:
      {
    KdPrint(("ImDisk: IOCTL_DISK_IS_WRITABLE for device.\n"/*,
         device_extension->device_number*/));

    if (!device_extension->read_only)
      ntStatus = STATUS_SUCCESS;
    else
      ntStatus = STATUS_MEDIA_WRITE_PROTECTED;

    irp->IoStatus.Information = 0;

    break;
      }

    case IOCTL_DISK_MEDIA_REMOVAL:
    case IOCTL_STORAGE_MEDIA_REMOVAL:
      {
    KdPrint(("ImDisk: IOCTL_DISK/STORAGE_MEDIA_REMOVAL for device.\n"/*,
         device_extension->device_number*/));

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    break;
      }

    case IOCTL_DISK_SET_PARTITION_INFO:
      {
    KdPrint(("ImDisk: IOCTL_DISK_SET_PARTITION_INFO for device.\n"/*,
         device_extension->device_number*/));

    if (device_extension->read_only)
      {
        KdPrint(("ImDisk: Attempt to partition read-only image.\n"));

        ntStatus = STATUS_MEDIA_WRITE_PROTECTED;
        irp->IoStatus.Information = 0;
        break;
      }

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(SET_PARTITION_INFORMATION))
      {
        ntStatus = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Information = 0;
        break;
      }

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    break;
      }

    case IOCTL_DISK_SET_PARTITION_INFO_EX:
      {
    PSET_PARTITION_INFORMATION_EX partition_information_ex;

    KdPrint(("ImDisk: IOCTL_DISK_SET_PARTITION_INFO_EX for device.\n"/*,
         device_extension->device_number*/));

    if (device_extension->read_only)
      {
        KdPrint(("ImDisk: Attempt to partition read-only image.\n"));

        ntStatus = STATUS_MEDIA_WRITE_PROTECTED;
        irp->IoStatus.Information = 0;
        break;
      }

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(SET_PARTITION_INFORMATION_EX))
      {
        ntStatus = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Information = 0;
        break;
      }

    partition_information_ex = (PSET_PARTITION_INFORMATION_EX)
      irp->AssociatedIrp.SystemBuffer;

    if (partition_information_ex->PartitionStyle != PARTITION_STYLE_MBR)
      {
        ntStatus = STATUS_UNSUCCESSFUL;
        irp->IoStatus.Information = 0;
      }
    else
      {
        ntStatus = STATUS_SUCCESS;
        irp->IoStatus.Information = 0;
      }

    break;
      }

    case IOCTL_DISK_VERIFY:
      {
    PVERIFY_INFORMATION verify_information;

    KdPrint(("ImDisk: IOCTL_DISK_VERIFY for device.\n"/*,
         device_extension->device_number*/));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VERIFY_INFORMATION))
      {
        ntStatus = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Information = 0;
        break;
      }

    verify_information = (PVERIFY_INFORMATION)
      irp->AssociatedIrp.SystemBuffer;

    if (device_extension->read_only)
      {
        KdPrint(("ImDisk: Attempt to verify read-only media.\n"));

        ntStatus = STATUS_MEDIA_WRITE_PROTECTED;
        irp->IoStatus.Information = 0;
        break;
      }

    if (verify_information->StartingOffset.QuadPart +
        verify_information->Length >
        device_extension->disk_geometry.Cylinders.QuadPart)
      {
        KdPrint(("ImDisk: Attempt to verify beyond image size.\n"));

        ntStatus = STATUS_NONEXISTENT_SECTOR;
        irp->IoStatus.Information = 0;
        break;
      }

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    break;
      }

    case IOCTL_STORAGE_GET_HOTPLUG_INFO:
      {
    PSTORAGE_HOTPLUG_INFO hotplug_info;

    KdPrint(("ImDisk: IOCTL_STORAGE_GET_HOTPLUG_INFO for device.\n"/*,
         device_extension->device_number*/));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(STORAGE_HOTPLUG_INFO))
      {
        ntStatus = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Information = 0;
        break;
      }

    hotplug_info = (PSTORAGE_HOTPLUG_INFO)
      irp->AssociatedIrp.SystemBuffer;

    hotplug_info->Size = sizeof(STORAGE_HOTPLUG_INFO);
    if (fdo->Characteristics & FILE_REMOVABLE_MEDIA)
      {
        hotplug_info->MediaRemovable = TRUE;
        hotplug_info->MediaHotplug = TRUE;
        hotplug_info->DeviceHotplug = TRUE;
        hotplug_info->WriteCacheEnableOverride = FALSE;
      }
    else
      {
        hotplug_info->MediaRemovable = FALSE;
        hotplug_info->MediaHotplug = FALSE;
        hotplug_info->DeviceHotplug = FALSE;
        hotplug_info->WriteCacheEnableOverride = FALSE;
      }

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);

    break;
      }

    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        {
            PMOUNTDEV_NAME mountDeviceName = irp->AssociatedIrp.SystemBuffer;
            int chars;

            KdPrint(("ImDisk: IOCTL_MOUNTDEV_QUERY_DEVICE_NAME for device.\n"/*,
                 device_extension->device_number*/));
            chars =
              _snwprintf(
                    mountDeviceName->Name,
                    (irpStack->Parameters.DeviceIoControl.OutputBufferLength - FIELD_OFFSET(MOUNTDEV_NAME, Name)) >> 1,
                    PUSH_RAMDISK_DEVICE_NAME
                    );

            if (chars < 0)
            {
                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
                FIELD_OFFSET(MOUNTDEV_NAME, Name) +
                sizeof(mountDeviceName->NameLength))
                  /*mountDeviceName->NameLength = sizeof(PUSH_DEVICE_BASE_NAME) +
                20;*/

                mountDeviceName->NameLength = sizeof(PUSH_RAMDISK_DEVICE_NAME) + 20;

                KdPrint(("ImDisk: IOCTL_MOUNTDEV_QUERY_DEVICE_NAME overflow, "
                     "buffer length %u, returned %i.\n",
                     irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                     chars));

                ntStatus = STATUS_BUFFER_OVERFLOW;

                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
                sizeof(MOUNTDEV_NAME))
                  irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                else
                  irp->IoStatus.Information = 0;

                break;
            }

    mountDeviceName->NameLength = (USHORT) chars << 1;

    ntStatus = STATUS_SUCCESS;
    irp->IoStatus.Information =
      FIELD_OFFSET(MOUNTDEV_NAME, Name) + mountDeviceName->NameLength;

    KdPrint(("[PUSH] => IOCTL_MOUNTDEV_QUERY_DEVICE_NAME => %ws, "
         "length %u total %u.\n",
         mountDeviceName->Name, mountDeviceName->NameLength,
         irp->IoStatus.Information));


      } break;

    default:
      {
            KdPrint(("[PUSH] Unknown IOCTL %#x.\n",
             irpStack->Parameters.DeviceIoControl.IoControlCode));

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            irp->IoStatus.Information = 0;
      }
    }

  if (ntStatus == STATUS_PENDING)
    {
      IoMarkIrpPending(irp);

      ExInterlockedInsertTailList(&device_extension->list_head,
                  &irp->Tail.Overlay.ListEntry,
                  &device_extension->list_lock);

      KeSetEvent(&device_extension->request_event, (KPRIORITY) 0, FALSE);
    }
  else
    {
      irp->IoStatus.Status = ntStatus;

      IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return ntStatus;
}


VOID
PushUnload( DRIVER_OBJECT* DriverObject )
{
    UNICODE_STRING DeviceLinkU;
    NTSTATUS ntStatus;
    PMAPINFO pMapInfo;
    PSINGLE_LIST_ENTRY pLink;

    DbgPrint("[Push] => (PushUnload)");

    RdUnload(DriverObject);

    //free resources
    pLink=PopEntryList(&lstMapInfo);
    while(pLink)
    {
        pMapInfo=CONTAINING_RECORD(pLink, MAPINFO, link);

        MmUnmapLockedPages(pMapInfo->pvu, pMapInfo->pMdl);
        IoFreeMdl(pMapInfo->pMdl);
        MmUnmapIoSpace(pMapInfo->pvk, pMapInfo->memSize);

        ExFreePool(pMapInfo);

        pLink=PopEntryList(&lstMapInfo);
    }


    //
    //  By default the I/O device is configured incorrectly or the
    // configuration parameters to the driver are incorrect.
    //
    ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;

    //
    // restore the call back routine, thus givinig chance to the
    // user mode application to unload dynamically the driver
    //
    ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);

    IoDeleteDevice(DriverObject->DeviceObject);

    RtlInitUnicodeString(&DeviceLinkU, PUSH_SYMLINK_NAME);

    ntStatus=IoDeleteSymbolicLink(&DeviceLinkU);

    if (NT_SUCCESS(ntStatus))
    {
        IoDeleteDevice(DriverObject->DeviceObject);
    }
    else
    {
        DbgPrint("Error: IoDeleteSymbolicLink failed");
    }

    DbgPrint("[Push] <= (PushUnload)");
}


NTSTATUS
MapPhysicalMemory(
    VOID* InputBuffer,
    ULONG InputBufferLength,
    ULONG OutputBufferLength,
    ULONG* BytesReturned
    )
{
    PHYSICAL_ADDRESS phyAddr;
    //PPHYMEM_MEM pMem;
    PHYSICAL_MEMORY *physicalMemory;
    PVOID pvk, pvu;

    //DbgPrint("[PUSH] MapPhysicalMemory");

    if (InputBufferLength != sizeof(PHYSICAL_MEMORY) || OutputBufferLength != sizeof(PVOID))
        return STATUS_INVALID_PARAMETER;

    physicalMemory=(PHYSICAL_MEMORY*)InputBuffer;
    phyAddr.QuadPart=(ULONGLONG)physicalMemory->pvAddr;

    //get mapped kernel address
    pvk=MmMapIoSpace(phyAddr, physicalMemory->dwSize, MmNonCached);

    if (pvk)
    {
        //allocate mdl for the mapped kernel address
        PMDL pMdl=IoAllocateMdl(pvk, physicalMemory->dwSize, FALSE, FALSE, NULL);
        if (pMdl)
        {
            PMAPINFO pMapInfo;

            //build mdl and map to user space
            MmBuildMdlForNonPagedPool(pMdl);
            pvu=MmMapLockedPages(pMdl, UserMode);

            //insert mapped infomation to list
            pMapInfo=(PMAPINFO)ExAllocatePool(\
                NonPagedPool, sizeof(MAPINFO));
            pMapInfo->pMdl=pMdl;
            pMapInfo->pvk=pvk;
            pMapInfo->pvu=pvu;
            pMapInfo->memSize=physicalMemory->dwSize;
            PushEntryList(&lstMapInfo, &pMapInfo->link);

            DbgPrint("Map physical 0x%x to virtual 0x%x, size %u", \
                physicalMemory->pvAddr, pvu, physicalMemory->dwSize);

            RtlCopyMemory(InputBuffer, &pvu, sizeof(PVOID));

            *BytesReturned = sizeof(PVOID);

            return STATUS_SUCCESS;
        }
        else
        {
            //allocate mdl error, unmap the mapped physical memory
            MmUnmapIoSpace(pvk, physicalMemory->dwSize);

            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
        return STATUS_INSUFFICIENT_RESOURCES;
}


NTSTATUS SetCacheName(void *lpInBuffer, ULONG nInBufferSize, void *lpOutBuffer, ULONG nOutBufferSize, ULONG *lpBytesReturned)
{
    strcpy(szCacheName, (char *)lpInBuffer);

    DbgPrint(szCacheName);

    return STATUS_SUCCESS;
}





