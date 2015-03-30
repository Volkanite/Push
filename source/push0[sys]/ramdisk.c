#include "push0.h"
#include "ramdisk.h"
//#include <initguid.h>

#pragma code_seg("PAGE")


NTSTATUS
ImDiskAddVirtualDisk(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT RAMDISK_CREATE_DATA* CreateData,
    IN PETHREAD ClientThread
    )
{
  NTSTATUS status;
  DEVICE_THREAD_DATA device_thread_data;
  HANDLE thread_handle;

  PAGED_CODE();

  ASSERT(DriverObject != NULL);
  ASSERT(CreateData != NULL);

  // Check if drive letter is disallowed by registry setting
  if (CreateData->DriveLetter != 0)
    {
      // Ensure upper-case driveletter.
      WCHAR ucase_drive_letter = CreateData->DriveLetter & ~0x20;

      if ((ucase_drive_letter >= L'A') & (ucase_drive_letter <= L'Z'))
    if (DisallowedDriveLetters[ucase_drive_letter - L'A'])
      return STATUS_ACCESS_DENIED;
    }

  device_thread_data.driver_object  = DriverObject;
  device_thread_data.create_data    = CreateData;
  device_thread_data.client_thread  = ClientThread;
  device_thread_data.caller_waiting = TRUE;

  KeInitializeEvent(&device_thread_data.created_event,
            NotificationEvent,
            FALSE);

  status = PsCreateSystemThread(&thread_handle,
                (ACCESS_MASK) 0L,
                NULL,
                NULL,
                NULL,
                ImDiskDeviceThread,
                &device_thread_data);

  if (!NT_SUCCESS(status))
    {
      KdPrint(("ImDisk: Cannot create device thread. (%#x)\n", status));

      return status;
    }

  ZwClose(thread_handle);

  KeWaitForSingleObject(&device_thread_data.created_event,
            Executive,
            KernelMode,
            FALSE,
            NULL);

  if (!NT_SUCCESS(device_thread_data.status))
    {
      KdPrint(("ImDisk: Device thread failed to initialize. (%#x)\n",
           device_thread_data.status));

      return device_thread_data.status;
    }

  if (CreateData->DriveLetter != 0)
    if (KeGetCurrentIrql() <= PASSIVE_LEVEL)
      ImDiskCreateDriveLetter(CreateData->DriveLetter);

  return STATUS_SUCCESS;
}


NTSTATUS
ImDiskCreateDriveLetter(IN wchar_t DriveLetter)
{
  WCHAR sym_link_global_wchar[] = L"\\DosDevices\\Global\\ :";
  WCHAR sym_link_wchar[] = L"\\DosDevices\\ :";
  UNICODE_STRING sym_link;
  //PWCHAR device_name_buffer;
  UNICODE_STRING deviceName;
  NTSTATUS status;

  DbgPrint("[PUSH] => (ImDiskCreateDriveLetter)");

  // Buffer for device name
  /*device_name_buffer = ExAllocatePoolWithTag(PagedPool,
                         MAXIMUM_FILENAME_LENGTH *
                         sizeof(*device_name_buffer),
                         POOL_TAG);*/

  /*if (device_name_buffer == NULL)
    {
      KdPrint(("ImDisk: Insufficient pool memory.\n"));
      return STATUS_INSUFFICIENT_RESOURCES;
    }*/

  /*_snwprintf(device_name_buffer, MAXIMUM_FILENAME_LENGTH - 1,
         PUSH_DEVICE_BASE_NAME L"%u",
         0);*/

  //device_name_buffer[MAXIMUM_FILENAME_LENGTH - 1] = 0;
  //RtlInitUnicodeString(&device_name, device_name_buffer);
    RtlInitUnicodeString(&deviceName, PUSH_RAMDISK_DEVICE_NAME);

    sym_link_wchar[12] = DriveLetter;

    KdPrint(("      Creating symlink '%ws' -> '%ws'.\n",
       sym_link_wchar, device_name_buffer));

    RtlInitUnicodeString(&sym_link, sym_link_wchar);

    status = IoCreateUnprotectedSymbolicLink(&sym_link, &deviceName);

    if (!NT_SUCCESS(status))
    {
      KdPrint(("ImDisk: Cannot symlink '%ws' to '%ws'. (%#x)\n",
           sym_link_global_wchar, device_name_buffer, status));
    }

    sym_link_global_wchar[19] = DriveLetter;

    KdPrint(("      Creating symlink '%ws' -> '%ws'.\n",
       sym_link_global_wchar, device_name_buffer));

    RtlInitUnicodeString(&sym_link, sym_link_global_wchar);

    status = IoCreateUnprotectedSymbolicLink(&sym_link, &deviceName);

    if (!NT_SUCCESS(status))
    {
      KdPrint(("ImDisk: Cannot symlink '%ws' to '%ws'. (%#x)\n",
           sym_link_global_wchar, device_name_buffer, status));
    }

    //ExFreePoolWithTag(device_name_buffer, POOL_TAG);

    DbgPrint("[PUSH] <= (ImDiskCreateDriveLetter)");

    return status;
}


// Parses BPB formatted geometry to a DISK_GEOMETRY structure.
VOID
ImDiskReadFormattedGeometry(IN OUT PDISK_GEOMETRY DiskGeometry,
                IN PFAT_BPB BPB)
{
  USHORT tmp;

  // Some sanity checks. Could this really be valid BPB values? Bytes per
  // sector is multiple of 2 etc?
  if (BPB->BytesPerSector == 0)
    return;

  if (BPB->SectorsPerTrack >= 64)
    return;

  if (BPB->NumberOfHeads >= 256)
    return;

  tmp = BPB->BytesPerSector;
  while ((tmp & 0x0001) == 0)
    tmp >>= 1;
  if ((tmp ^ 0x0001) != 0)
    return;

  if (DiskGeometry->SectorsPerTrack == 0)
    DiskGeometry->SectorsPerTrack = BPB->SectorsPerTrack;

  if (DiskGeometry->TracksPerCylinder == 0)
    DiskGeometry->TracksPerCylinder = BPB->NumberOfHeads;

  if (DiskGeometry->BytesPerSector == 0)
    DiskGeometry->BytesPerSector = BPB->BytesPerSector;
}


INT32
ImDiskCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT RAMDISK_CREATE_DATA* CreateData,
    IN PETHREAD ClientThread,
    OUT PDEVICE_OBJECT *DeviceObject
    )
{
    //UNICODE_STRING    file_name;
    UNICODE_STRING    deviceName;
    NTSTATUS          status;
    PDEVICE_EXTENSION device_extension;
    DEVICE_TYPE       device_type;
    ULONG             device_characteristics;
    HANDLE file_handle = NULL;
    PUCHAR image_buffer = NULL;
    PROXY_CONNECTION proxy = { 0 };
    ULONG alignment_requirement;
    LONGLONG calccyl;
    SIZE_T max_size;
    ULONG flags = 0;
    USHORT fileNameLength = 0;

    PAGED_CODE();

  ASSERT(CreateData != NULL);

  KdPrint
    (("ImDisk: Got request to create a virtual disk. Request data:\n"
      "DiskGeometry\n"
      "  .Cylinders   = 0x%.8x%.8x\n"
      "  .MediaType   = %i\n"
      "  .T/C         = %u\n"
      "  .S/T         = %u\n"
      "  .B/S         = %u\n"
      "Offset         = 0x%.8x%.8x\n"
      "Flags          = %#x\n"
      "FileNameLength = %u\n"
      "FileName       = '%.*ws'\n"
      "DriveLetter    = %wc\n",
      CreateData->DiskGeometry.Cylinders.HighPart,
      CreateData->DiskGeometry.Cylinders.LowPart,
      CreateData->DiskGeometry.MediaType,
      CreateData->DiskGeometry.TracksPerCylinder,
      CreateData->DiskGeometry.SectorsPerTrack,
      CreateData->DiskGeometry.BytesPerSector,
      CreateData->ImageOffset.HighPart,
      CreateData->ImageOffset.LowPart,
      CreateData->Flags,
      CreateData->FileNameLength,
      (int)(CreateData->FileNameLength / sizeof(*CreateData->FileName)),
      CreateData->FileName,
      CreateData->DriveLetter));

    // Auto-select type if not specified.
    if (IMDISK_TYPE(flags) == 0)
    {
        if (fileNameLength == 0)
            flags |= IMDISK_TYPE_VM;
        else
            flags |= IMDISK_TYPE_FILE;
    }

  // Blank filenames only supported for non-zero VM disks.
  if (((fileNameLength == 0) &
       (IMDISK_TYPE(flags) != IMDISK_TYPE_VM)) |
      ((fileNameLength == 0) &
       (IMDISK_TYPE(flags) == IMDISK_TYPE_VM) &
       (CreateData->DiskGeometry.Cylinders.QuadPart == 0)))
    {
      KdPrint(("ImDisk: Blank filenames only supported for non-zero length "
           "vm type disks.\n"));

      ImDiskLogError((DriverObject,
              0,
              0,
              NULL,
              0,
              1000,
              STATUS_INVALID_PARAMETER,
              102,
              STATUS_INVALID_PARAMETER,
              0,
              0,
              NULL,
              L"Blank filenames only supported for non-zero length "
              L"vm type disks."));

      return STATUS_INVALID_PARAMETER;
    }

  // Cannot create >= 2 GB VM disk in 32 bit version.
#ifndef _AMD64_
  if ((IMDISK_TYPE(flags) == IMDISK_TYPE_VM) &
      ((CreateData->DiskGeometry.Cylinders.QuadPart & 0xFFFFFFFF80000000) !=
       0))
    {
      KdPrint(("ImDisk: Cannot create >= 2GB vm disks on 32-bit system.\n"));

      return STATUS_INVALID_PARAMETER;
    }
#endif

#ifdef _AMD64_
      max_size = CreateData->DiskGeometry.Cylinders.QuadPart;
#else
      max_size = CreateData->DiskGeometry.Cylinders.LowPart;
#endif

      image_buffer = NULL;
      status =
        ZwAllocateVirtualMemory(NtCurrentProcess(),
                                &image_buffer,
                                0,
                                &max_size,
                                MEM_COMMIT,
                                PAGE_READWRITE);

        if (!NT_SUCCESS(status))
        {
            KdPrint
            (("PUSH: Error allocating virtual memory for vm disk (%#x).\n",
          status));

            return STATUS_NO_MEMORY;
        }

      alignment_requirement = FILE_BYTE_ALIGNMENT;
    //}

  KdPrint(("ImDisk: Done with file/memory checks.\n"));

    flags |= IMDISK_DEVICE_TYPE_HD;

  // If some parts of the DISK_GEOMETRY structure are zero, auto-fill with
  // typical values for this type of disk.

      calccyl = CreateData->DiskGeometry.Cylinders.QuadPart;

      // In this case the Cylinders member actually specifies the total size of
      // the virtual disk so restore that in case overwritten by the pre-
      // defined floppy geometries above.
      CreateData->DiskGeometry.Cylinders.QuadPart = calccyl;

      if (CreateData->DiskGeometry.BytesPerSector == 0)
    CreateData->DiskGeometry.BytesPerSector = SECTOR_SIZE_HDD;

      calccyl /= CreateData->DiskGeometry.BytesPerSector;

      if (CreateData->DiskGeometry.SectorsPerTrack == 0)
    CreateData->DiskGeometry.SectorsPerTrack = HEAD_SIZE_HDD;

      calccyl /= CreateData->DiskGeometry.SectorsPerTrack;

      if (CreateData->DiskGeometry.TracksPerCylinder == 0)
    {
      CreateData->DiskGeometry.TracksPerCylinder = 1;

      if (calccyl >= 130560)
        {
          CreateData->DiskGeometry.TracksPerCylinder = 255;
          calccyl /= 255;
        }
      else
        while ((calccyl > 128) &
           (CreateData->DiskGeometry.TracksPerCylinder < 128))
          {
        CreateData->DiskGeometry.TracksPerCylinder <<= 1;
        calccyl >>= 1;
          }
    }

      if (CreateData->DiskGeometry.MediaType == Unknown)
    CreateData->DiskGeometry.MediaType = FixedMedia;


  KdPrint(("ImDisk: Done with disk geometry setup.\n"));

  // Ensure upper-case driveletter.
  CreateData->DriveLetter &= ~0x20;

  // Now build real DeviceType and DeviceCharacteristics parameters.
  if (IMDISK_DEVICE_TYPE(flags) == IMDISK_DEVICE_TYPE_CD)
    {
      device_type = FILE_DEVICE_CD_ROM;
      device_characteristics = FILE_READ_ONLY_DEVICE | FILE_REMOVABLE_MEDIA;
    }
  else
    {
      device_type = FILE_DEVICE_DISK;

      if (IMDISK_DEVICE_TYPE(flags) == IMDISK_DEVICE_TYPE_FD)
    device_characteristics = FILE_FLOPPY_DISKETTE | FILE_REMOVABLE_MEDIA;
      else
    device_characteristics = 0;
    }

  if (IMDISK_REMOVABLE(flags))
    device_characteristics |= FILE_REMOVABLE_MEDIA;

  if (IMDISK_READONLY(flags))
    device_characteristics |= FILE_READ_ONLY_DEVICE;

  KdPrint
    (("ImDisk: After checks and translations we got this create data:\n"
      //"DeviceNumber   = %#x\n"
      "DiskGeometry\n"
      "  .Cylinders   = 0x%.8x%.8x\n"
      "  .MediaType   = %i\n"
      "  .T/C         = %u\n"
      "  .S/T         = %u\n"
      "  .B/S         = %u\n"
      "Offset         = 0x%.8x%.8x\n"
      "Flags          = %#x\n"
      "FileNameLength = %u\n"
      "FileName       = '%.*ws'\n"
      "DriveLetter    = %wc\n",
      //CreateData->DeviceNumber,
      CreateData->DiskGeometry.Cylinders.HighPart,
      CreateData->DiskGeometry.Cylinders.LowPart,
      CreateData->DiskGeometry.MediaType,
      CreateData->DiskGeometry.TracksPerCylinder,
      CreateData->DiskGeometry.SectorsPerTrack,
      CreateData->DiskGeometry.BytesPerSector,
      CreateData->ImageOffset.HighPart,
      CreateData->ImageOffset.LowPart,
      CreateData->Flags,
      CreateData->FileNameLength,
      (int)(CreateData->FileNameLength / sizeof(*CreateData->FileName)),
      CreateData->FileName,
      CreateData->DriveLetter));

  // Buffer for device name
  /*device_name_buffer = ExAllocatePoolWithTag(PagedPool,
                         MAXIMUM_FILENAME_LENGTH *
                         sizeof(*device_name_buffer),
                         POOL_TAG);*/

  /*if (device_name_buffer == NULL)
    {
      SIZE_T free_size = 0;
      ImDiskCloseProxy(&proxy);
      if (file_handle != NULL)
    ZwClose(file_handle);
      if (file_name.Buffer != NULL)
    ExFreePoolWithTag(file_name.Buffer, POOL_TAG);
      if (image_buffer != NULL)
    ZwFreeVirtualMemory(NtCurrentProcess(),
                &image_buffer,
                &free_size, MEM_RELEASE);

      ImDiskLogError((DriverObject,
              0,
              0,
              NULL,
              0,
              1000,
              STATUS_INSUFFICIENT_RESOURCES,
              102,
              STATUS_INSUFFICIENT_RESOURCES,
              0,
              0,
              NULL,
              L"Memory allocation error."));

      return STATUS_INSUFFICIENT_RESOURCES;
    }*/

  /*_snwprintf(device_name_buffer, MAXIMUM_FILENAME_LENGTH - 1,
         PUSH_DEVICE_BASE_NAME L"%u", 0);
  device_name_buffer[MAXIMUM_FILENAME_LENGTH - 1] = 0;*/

  //RtlInitUnicodeString(&device_name, device_name_buffer);
    RtlInitUnicodeString(&deviceName, PUSH_RAMDISK_DEVICE_NAME);

  // Driver can no longer be completely paged.
  KdPrint(("ImDisk: Resetting driver paging.\n"));

  MmResetDriverPaging((PVOID)(ULONG_PTR) DriverEntry);

  KdPrint
    (("ImDisk: Creating device '%ws'. Device type %#x, characteristics %#x.\n",
      device_name_buffer, device_type, device_characteristics));

  status = IoCreateDevice(DriverObject,
              sizeof(DEVICE_EXTENSION),
              &deviceName,
              device_type,
              device_characteristics,
              FALSE,
              DeviceObject);

  if (!NT_SUCCESS(status))
    {
      SIZE_T free_size = 0;

      //ExFreePoolWithTag(device_name_buffer, POOL_TAG);
      ImDiskCloseProxy(&proxy);
      if (file_handle != NULL)
    ZwClose(file_handle);
      /*if (file_name.Buffer != NULL)
    ExFreePoolWithTag(file_name.Buffer, POOL_TAG);*/
      if (image_buffer != NULL)
    ZwFreeVirtualMemory(NtCurrentProcess(),
                &image_buffer,
                &free_size, MEM_RELEASE);

      ImDiskLogError((DriverObject,
              0,
              0,
              NULL,
              0,
              1000,
              status,
              102,
              status,
              0,
              0,
              NULL,
              L"Error creating device object."));

      KdPrint(("ImDisk: Cannot create device. (%#x)\n", status));

      return status;
    }

  KdPrint
    (("ImDisk: Setting the AlignmentRequirement field to %#x.\n",
      alignment_requirement));

  (*DeviceObject)->Flags |= DO_DIRECT_IO;
  (*DeviceObject)->AlignmentRequirement = alignment_requirement;

  device_extension = (PDEVICE_EXTENSION) (*DeviceObject)->DeviceExtension;

  // Auto-set our own read-only flag if the characteristics of the device
  // object is set to read-only.
  if ((*DeviceObject)->Characteristics & FILE_READ_ONLY_DEVICE)
    device_extension->read_only = TRUE;

  InitializeListHead(&device_extension->list_head);

  KeInitializeSpinLock(&device_extension->list_lock);

  KeInitializeEvent(&device_extension->request_event,
            SynchronizationEvent, FALSE);

  KeInitializeEvent(&device_extension->terminate_thread,
            NotificationEvent, FALSE);

  //device_extension->device_number = 0;

  //DeviceList |= 1ULL << CreateData->DeviceNumber;

  //device_extension->file_name = file_name;

  device_extension->disk_geometry = CreateData->DiskGeometry;

  //device_extension->image_offset = CreateData->ImageOffset;

  if (IMDISK_SPARSE_FILE(flags))
    device_extension->use_set_zero_data = TRUE;

  // VM disk.
  if (IMDISK_TYPE(flags) == IMDISK_TYPE_VM)
    device_extension->vm_disk = TRUE;
  else
    device_extension->vm_disk = FALSE;

  device_extension->image_buffer = image_buffer;
  device_extension->file_handle = file_handle;

  // Use proxy service.
  if (IMDISK_TYPE(flags) == IMDISK_TYPE_PROXY)
    {
      device_extension->proxy = proxy;
      device_extension->use_proxy = TRUE;
    }
  else
    device_extension->use_proxy = FALSE;

  if (((*DeviceObject)->DeviceType == FILE_DEVICE_CD_ROM) |
      IMDISK_READONLY(flags))
    device_extension->read_only = TRUE;
  else
    device_extension->read_only = FALSE;

  if (device_extension->read_only)
    (*DeviceObject)->Characteristics |= FILE_READ_ONLY_DEVICE;
  else
    (*DeviceObject)->Characteristics &= ~FILE_READ_ONLY_DEVICE;

  device_extension->media_change_count++;

  device_extension->drive_letter = CreateData->DriveLetter;

  device_extension->device_thread = KeGetCurrentThread();

  (*DeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;

  KdPrint(("ImDisk: Device '%ws' created.\n", device_name_buffer));

  //ExFreePoolWithTag(device_name_buffer, POOL_TAG);

  return STATUS_SUCCESS;
}


VOID
RdUnload(IN PDRIVER_OBJECT DriverObject)
{
  PDEVICE_OBJECT device_object;

  PAGED_CODE();

  device_object = DriverObject->DeviceObject;

  KdPrint(("ImDisk: Entering ImDiskUnload for driver %#x. "
       "Current device objects chain dump for this driver:\n",
       DriverObject));

  while (device_object != NULL)
    {
      KdPrint(("%#x -> ", device_object));
      device_object = device_object->NextDevice;
    }

  KdPrint(("(null)\n"));

  device_object = DriverObject->DeviceObject;

  for (;;)
    {
      PDEVICE_OBJECT next_device;
      PDEVICE_EXTENSION device_extension;

      if (device_object == NULL)
    {
      KdPrint
        (("ImDisk: No more devices to delete. Leaving ImDiskUnload.\n"));
      return;
    }

      next_device = device_object->NextDevice;
      device_extension = (PDEVICE_EXTENSION) device_object->DeviceExtension;

      KdPrint(("ImDisk: Now deleting device.\n"/*,
           device_extension->device_number*/));

      if (device_object == g_DeviceObject)
    {
      UNICODE_STRING sym_link;
      LARGE_INTEGER time_out;
      time_out.QuadPart = -1000000;

      /*while (device_object->ReferenceCount != 0)
        {
          KdPrint(("ImDisk: Ctl device is busy. Waiting.\n"));

          KeDelayExecutionThread(KernelMode, FALSE, &time_out);

          time_out.LowPart <<= 2;
        }*/

      KdPrint(("ImDisk: Deleting ctl device.\n"));
      RtlInitUnicodeString(&sym_link, PUSH_SYMLINK_NAME);
      IoDeleteSymbolicLink(&sym_link);
      IoDeleteDevice(device_object);
    }
      else
    {
      PKTHREAD device_thread;

      KdPrint(("ImDisk: Shutting down device.\n"/*,
           device_extension->device_number*/));

      device_thread = device_extension->device_thread;
      ObReferenceObjectByPointer(device_thread, SYNCHRONIZE, NULL,
                     KernelMode);

      ImDiskRemoveVirtualDisk(device_object);

      KdPrint(("ImDisk: Waiting for device thread to terminate.\n"/*,
           device_extension->device_number*/));

      KeWaitForSingleObject(device_thread,
                Executive,
                KernelMode,
                FALSE,
                NULL);

      ObDereferenceObject(device_thread);
    }

      device_object = next_device;
    }
}

#pragma code_seg()

#if DEBUG_LEVEL >= 1
VOID
ImDiskLogDbgError(IN PVOID Object,
          IN UCHAR MajorFunctionCode,
          IN UCHAR RetryCount,
          IN PULONG DumpData,
          IN USHORT DumpDataSize,
          IN USHORT EventCategory,
          IN NTSTATUS ErrorCode,
          IN ULONG UniqueErrorValue,
          IN NTSTATUS FinalStatus,
          IN ULONG SequenceNumber,
          IN ULONG IoControlCode,
          IN PLARGE_INTEGER DeviceOffset,
          IN PWCHAR Message)
{
  ULONG_PTR string_byte_size;
  ULONG_PTR packet_size;
  PIO_ERROR_LOG_PACKET error_log_packet;

  if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    return;

  string_byte_size = (wcslen(Message) + 1) << 1;

  packet_size =
    sizeof(IO_ERROR_LOG_PACKET) + DumpDataSize + string_byte_size;

  if (packet_size > ERROR_LOG_MAXIMUM_SIZE)
    {
      KdPrint(("ImDisk: Warning: Too large error log packet.\n"));
      return;
    }

  error_log_packet =
    (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(Object,
                           (UCHAR) packet_size);

  if (error_log_packet == NULL)
    {
      KdPrint(("ImDisk: Warning: IoAllocateErrorLogEntry() returned NULL.\n"));
      return;
    }

  error_log_packet->MajorFunctionCode = MajorFunctionCode;
  error_log_packet->RetryCount = RetryCount;
  error_log_packet->StringOffset = sizeof(IO_ERROR_LOG_PACKET) + DumpDataSize;
  error_log_packet->EventCategory = EventCategory;
  error_log_packet->ErrorCode = ErrorCode;
  error_log_packet->UniqueErrorValue = UniqueErrorValue;
  error_log_packet->FinalStatus = FinalStatus;
  error_log_packet->SequenceNumber = SequenceNumber;
  error_log_packet->IoControlCode = IoControlCode;
  if (DeviceOffset != NULL)
    error_log_packet->DeviceOffset = *DeviceOffset;
  error_log_packet->DumpDataSize = DumpDataSize;

  if (DumpDataSize != 0)
    memcpy(error_log_packet->DumpData, DumpData, DumpDataSize);

  if (Message == NULL)
    error_log_packet->NumberOfStrings = 0;
  else
    {
      error_log_packet->NumberOfStrings = 1;
      memcpy((PUCHAR)error_log_packet + error_log_packet->StringOffset,
         Message,
         string_byte_size);
    }

  IoWriteErrorLogEntry(error_log_packet);
}
#endif

VOID
ImDiskRemoveVirtualDisk(IN PDEVICE_OBJECT DeviceObject)
{
  PDEVICE_EXTENSION device_extension;

  ASSERT(DeviceObject != NULL);

  device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

  KdPrint(("ImDisk: Request to shutdown device.\n"/*,
       device_extension->device_number*/));

  if (device_extension->drive_letter != 0)
    if (KeGetCurrentIrql() <= PASSIVE_LEVEL)
      ImDiskRemoveDriveLetter(device_extension->drive_letter);

  KeSetEvent(&device_extension->terminate_thread, (KPRIORITY) 0, FALSE);
}

NTSTATUS
ImDiskRemoveDriveLetter(IN WCHAR DriveLetter)
{
  NTSTATUS status;
  WCHAR sym_link_global_wchar[] = L"\\DosDevices\\Global\\ :";
  WCHAR sym_link_wchar[] = L"\\DosDevices\\ :";
  UNICODE_STRING sym_link;

  sym_link_global_wchar[19] = DriveLetter;

  KdPrint(("ImDisk: Removing symlink '%ws'.\n", sym_link_global_wchar));

  RtlInitUnicodeString(&sym_link, sym_link_global_wchar);
  status = IoDeleteSymbolicLink(&sym_link);

  if (!NT_SUCCESS(status))
    {
      KdPrint
    (("ImDisk: Cannot remove symlink '%ws'. (%#x)\n",
      sym_link_global_wchar, status));
    }

  sym_link_wchar[12] = DriveLetter;

  KdPrint(("ImDisk: Removing symlink '%ws'.\n", sym_link_wchar));

  RtlInitUnicodeString(&sym_link, sym_link_wchar);
  status = IoDeleteSymbolicLink(&sym_link);

  if (!NT_SUCCESS(status))
    KdPrint(("ImDisk: Cannot remove symlink '%ws'. (%#x)\n",
         sym_link_wchar, status));

  return status;
}

NTSTATUS
ImDiskCreateClose(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
  PIO_STACK_LOCATION io_stack;
  PDEVICE_EXTENSION device_extension;
  NTSTATUS status;

  ASSERT(DeviceObject != NULL);
  ASSERT(Irp != NULL);

  KdPrint(("ImDisk: Entering ImDiskCreateClose.\n"));

  io_stack = IoGetCurrentIrpStackLocation(Irp);
  device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

  if (io_stack->FileObject->FileName.Length != 0)
    {
      KdPrint(("ImDisk: Attempt to open '%.*ws' on device.\n",
           (int)(io_stack->FileObject->FileName.Length / sizeof(WCHAR)),
           io_stack->FileObject->FileName.Buffer/*,
           device_extension->device_number*/));

      status = STATUS_OBJECT_NAME_NOT_FOUND;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return status;
    }

  if ((io_stack->MajorFunction == IRP_MJ_CREATE) &
      (KeReadStateEvent(&device_extension->terminate_thread) != 0))
    {
      KdPrint(("ImDisk: Attempt to open device when shut down.\n"/*,
           device_extension->device_number*/));

      status = STATUS_DELETE_PENDING;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return status;
    }

  KdPrint(("ImDisk: Successfully created/closed a handle for device.\n"/*,
       device_extension->device_number*/));

  status = STATUS_SUCCESS;

  Irp->IoStatus.Status = status;
  Irp->IoStatus.Information = FILE_OPENED;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return status;
}

NTSTATUS
ImDiskReadWrite(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp)
{
  PDEVICE_EXTENSION device_extension;
  PIO_STACK_LOCATION io_stack;
  NTSTATUS status;

  ASSERT(DeviceObject != NULL);
  ASSERT(Irp != NULL);

  device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

  if (DeviceObject == g_DeviceObject)
    {
      KdPrint(("ImDisk: Read/write attempt on ctl device.\n"));

      status = STATUS_INVALID_DEVICE_REQUEST;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return status;
    }

  if (KeReadStateEvent(&device_extension->terminate_thread) != 0)
    {
      KdPrint(("ImDisk: Read/write attempt on device while removing.\n"/*,
           device_extension->device_number*/));

      status = STATUS_NO_MEDIA_IN_DEVICE;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return status;
    }

  io_stack = IoGetCurrentIrpStackLocation(Irp);

  if ((io_stack->MajorFunction == IRP_MJ_WRITE) &&
      device_extension->read_only)
    {
      KdPrint(("ImDisk: Attempt to write to write-protected device.\n"/*,
           device_extension->device_number*/));

      status = STATUS_MEDIA_WRITE_PROTECTED;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return status;
    }

  if ((io_stack->Parameters.Read.ByteOffset.QuadPart +
       io_stack->Parameters.Read.Length) >
      (device_extension->disk_geometry.Cylinders.QuadPart))
    {
      KdPrint(("ImDisk: Read/write beyond eof on device.\n"/*,
           device_extension->device_number*/));

      status = STATUS_SUCCESS;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return status;
    }

  if (io_stack->Parameters.Read.Length == 0)
    {
      KdPrint(("ImDisk: Read/write zero bytes on device.\n"/*,
           device_extension->device_number*/));

      status = STATUS_SUCCESS;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return status;
    }

  KdPrint2(("ImDisk: Device %i got read/write request Offset=%p%p Len=%p.\n",
        device_extension->device_number,
        io_stack->Parameters.Read.ByteOffset.HighPart,
        io_stack->Parameters.Read.ByteOffset.LowPart,
        io_stack->Parameters.Read.Length));

  IoMarkIrpPending(Irp);

  ExInterlockedInsertTailList(&device_extension->list_head,
                  &Irp->Tail.Overlay.ListEntry,
                  &device_extension->list_lock);

  KeSetEvent(&device_extension->request_event, (KPRIORITY) 0, FALSE);

  return STATUS_PENDING;
}



NTSTATUS
ImDiskDispatchPnP(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
  PDEVICE_EXTENSION device_extension;
  PIO_STACK_LOCATION io_stack;
  NTSTATUS status;

  ASSERT(DeviceObject != NULL);
  ASSERT(Irp != NULL);

  device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

  io_stack = IoGetCurrentIrpStackLocation(Irp);

  KdPrint(("ImDisk: Device received PnP function %#x IRP %#x.\n",
       //device_extension->device_number,
       io_stack->MinorFunction,
       Irp));

  if (KeReadStateEvent(&device_extension->terminate_thread) != 0)
    {
      KdPrint(("ImDisk: PnP dispatch on device that is being removed.\n"/*,
           device_extension->device_number*/));

      status = STATUS_DELETE_PENDING;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return status;
    }

  // The control device cannot receive PnP dispatch.
  if (DeviceObject == g_DeviceObject)
    {
      KdPrint(("ImDisk: PnP function %#x invalid for control device.\n",
           io_stack->MinorFunction));

      status = STATUS_INVALID_DEVICE_REQUEST;

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;

      return status;
    }

  switch (io_stack->MinorFunction)
    {
    case IRP_MN_DEVICE_USAGE_NOTIFICATION:
      KdPrint(("ImDisk: Device got IRP_MN_DEVICE_USAGE_NOTIFICATION.\n",
           //device_extension->device_number,
           io_stack->MinorFunction,
           Irp));

      switch (io_stack->Parameters.UsageNotification.Type)
    {
    case DeviceUsageTypePaging:
    case DeviceUsageTypeDumpFile:
      if (device_extension->read_only)
        status = STATUS_MEDIA_WRITE_PROTECTED;
      else
        if (io_stack->Parameters.UsageNotification.InPath == TRUE)
          MmLockPagableCodeSection((PVOID)(ULONG_PTR) ImDiskDeviceThread);

        IoAdjustPagingPathCount
          (&device_extension->special_file_count,
           io_stack->Parameters.UsageNotification.InPath);
        status = STATUS_SUCCESS;

      break;

    default:
      status = STATUS_NOT_SUPPORTED;
    }

      break;

    default:
      KdPrint(("ImDisk: Unknown PnP function %#x.\n",
           io_stack->MinorFunction));

      status = STATUS_INVALID_DEVICE_REQUEST;
      Irp->IoStatus.Information = 0;
    }

  if (status == STATUS_PENDING)
    {
      IoMarkIrpPending(Irp);

      ExInterlockedInsertTailList(&device_extension->list_head,
                  &Irp->Tail.Overlay.ListEntry,
                  &device_extension->list_lock);

      KeSetEvent(&device_extension->request_event, (KPRIORITY) 0, FALSE);
    }
  else
    {
      Irp->IoStatus.Status = status;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

  return status;
}

#pragma code_seg("PAGE")

VOID
ImDiskDeviceThread(IN PVOID Context)
{
  PDEVICE_THREAD_DATA device_thread_data;
  PDEVICE_OBJECT device_object;
  PDEVICE_EXTENSION device_extension;
  LARGE_INTEGER time_out;
  BOOLEAN system_drive_letter;

  PAGED_CODE();

  ASSERT(Context != NULL);

  KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

  device_thread_data = (PDEVICE_THREAD_DATA) Context;

  system_drive_letter = !device_thread_data->caller_waiting;

  // This is in case this thread is created by
  // ImDiskAddVirtualDiskAfterInitialization() when called from DriverEntry().
  // That indicates that no-one is waiting for us to return any status
  // in the device_thread_data members and that there is no-one freeing the
  // init structures after we are finished with them.
  // It also means that we need to wait for the control device to get ready for
  // I/O (in case a proxy or something need to call this driver during device
  // initialization).
  while (g_DeviceObject->Flags & DO_DEVICE_INITIALIZING)
    {
      LARGE_INTEGER wait_time;

      KdPrint2(("ImDisk: Driver still initializing, waiting 100 ms...\n"));

      wait_time.QuadPart = -1000000;
      KeDelayExecutionThread(KernelMode, FALSE, &wait_time);
    }

  device_thread_data->status =
    ImDiskCreateDevice(device_thread_data->driver_object,
               device_thread_data->create_data,
               device_thread_data->client_thread,
               &device_object);

  g_pDiskDeviceObject = device_object;

  if (!NT_SUCCESS(device_thread_data->status))
    {
      if (device_thread_data->caller_waiting)
    KeSetEvent(&device_thread_data->created_event, (KPRIORITY) 0, FALSE);
      else
    {
      ExFreePoolWithTag(device_thread_data->create_data, POOL_TAG);
      ExFreePoolWithTag(device_thread_data, POOL_TAG);
    }

      ImDiskLogError((device_thread_data->driver_object,
              0,
              0,
              NULL,
              0,
              1000,
              device_thread_data->status,
              102,
              device_thread_data->status,
              0,
              0,
              NULL,
              L"Error creating virtual disk."));

      PsTerminateSystemThread(STATUS_SUCCESS);
    }

  // Now we are done with initialization. Let the one that asks us to create
  // this device now that, or if no-one left there, clean up init structures
  // here.
  if (device_thread_data->caller_waiting)
    KeSetEvent(&device_thread_data->created_event, (KPRIORITY) 0, FALSE);
  else
    {
      ImDiskCreateDriveLetter(device_thread_data->create_data->DriveLetter);

      ExFreePoolWithTag(device_thread_data->create_data, POOL_TAG);
      ExFreePoolWithTag(device_thread_data, POOL_TAG);
    }

  KdPrint(("ImDisk: Device thread initialized. (flags=%#x)\n",
       device_object->Flags));

  device_extension = (PDEVICE_EXTENSION) device_object->DeviceExtension;

  time_out.QuadPart = -1000000;

  // If this is a VM backed disk that should be pre-loaded with an image file
  // we have to load the contents of that file now before entering the service
  // loop.
  if (device_extension->vm_disk && (device_extension->file_handle != NULL))
    {
      LARGE_INTEGER byte_offset = device_extension->image_offset;
      IO_STATUS_BLOCK io_status;
      NTSTATUS status;
#ifdef _AMD64_
      SIZE_T disk_size = device_extension->disk_geometry.Cylinders.QuadPart;
#else
      SIZE_T disk_size = device_extension->disk_geometry.Cylinders.LowPart;
#endif

      KdPrint(("ImDisk: Reading image file into vm disk buffer.\n"));

      status =
    ImDiskSafeReadFile(device_extension->file_handle,
               &io_status,
               device_extension->image_buffer,
               disk_size,
               &byte_offset);

      ZwClose(device_extension->file_handle);
      device_extension->file_handle = NULL;

      // Failure to read pre-load image is now considered a fatal error
      if (!NT_SUCCESS(status))
    {
      KdPrint(("ImDisk: Failed to read image file (%#x).\n", status));

      ImDiskRemoveVirtualDisk(device_object);
    }
      else
    KdPrint(("ImDisk: Image loaded successfully.\n"));
    }

  for (;;)
    {
      PIRP irp;
      PIO_STACK_LOCATION io_stack;
      PLIST_ENTRY request =
    ExInterlockedRemoveHeadList(&device_extension->list_head,
                    &device_extension->list_lock);

      if (request == NULL)
    {
      LARGE_INTEGER buffer_timeout;
      PLARGE_INTEGER p_wait_timeout;
      NTSTATUS status;
      PKEVENT wait_objects[] = {
        &device_extension->request_event,
        &device_extension->terminate_thread
      };

      // I/O double buffer timeout 5 sec
      if (device_extension->last_io_data != NULL)
        {
          buffer_timeout.QuadPart = -50000000;
          p_wait_timeout = &buffer_timeout;
        }
      else
        p_wait_timeout = NULL;


      KdPrint2(("ImDisk: No pedning requests. Waiting.\n"));

      status = KeWaitForMultipleObjects(sizeof(wait_objects) /
                        sizeof(*wait_objects),
                        wait_objects,
                        WaitAny,
                        Executive,
                        KernelMode,
                        FALSE,
                        p_wait_timeout,
                        NULL);

      // Free double buffer if timeout
      if (status == STATUS_TIMEOUT)
        {
          ExFreePoolWithTag(device_extension->last_io_data, POOL_TAG);
          device_extension->last_io_data = NULL;

          continue;
        }

      // While pending requests in queue, service them before terminating
      // thread.
      if (status == STATUS_WAIT_0)
        continue;

      KdPrint(("ImDisk: Device thread is shutting down.\n"/*,
           device_extension->device_number*/));

      if (device_extension->drive_letter != 0)
        if (system_drive_letter)
          ImDiskRemoveDriveLetter(device_extension->drive_letter);

      ImDiskCloseProxy(&device_extension->proxy);

      if (device_extension->last_io_data != NULL)
        {
          ExFreePoolWithTag(device_extension->last_io_data, POOL_TAG);
          device_extension->last_io_data = NULL;
        }

      if (device_extension->vm_disk)
        {
          SIZE_T free_size = 0;
          if (device_extension->image_buffer != NULL)
        ZwFreeVirtualMemory(NtCurrentProcess(),
                    &device_extension->image_buffer,
                    &free_size, MEM_RELEASE);

          device_extension->image_buffer = NULL;
        }
      else
        {
          if (device_extension->file_handle != NULL)
        ZwClose(device_extension->file_handle);

          device_extension->file_handle = NULL;
        }

      if (device_extension->file_name.Buffer != NULL)
        {
          ExFreePoolWithTag(device_extension->file_name.Buffer, POOL_TAG);
          device_extension->file_name.Buffer = NULL;
          device_extension->file_name.Length = 0;
          device_extension->file_name.MaximumLength = 0;
        }

      // If ReferenceCount is not zero, this device may have outstanding
      // IRP-s or otherwise unfinished things to do. Let IRP-s be done by
      // continuing this dispatch loop until ReferenceCount is zero.
      /*if (device_object->ReferenceCount != 0)
        {
          KdPrint(("ImDisk: Device has %i references. Waiting.\n",
               //device_extension->device_number,
               device_object->ReferenceCount));

          KeDelayExecutionThread(KernelMode, FALSE, &time_out);

          time_out.LowPart <<= 4;
          continue;
        }*/

      KdPrint(("ImDisk: Deleting device object.\n"/*,
           device_extension->device_number*/));

      //DeviceList &= ~(1ULL << device_extension->device_number);

      IoDeleteDevice(device_object);

      PsTerminateSystemThread(STATUS_SUCCESS);
    }

      irp = CONTAINING_RECORD(request, IRP, Tail.Overlay.ListEntry);

      io_stack = IoGetCurrentIrpStackLocation(irp);

      switch (io_stack->MajorFunction)
    {
    case IRP_MJ_READ:
      {
        PUCHAR system_buffer =
          (PUCHAR) MmGetSystemAddressForMdlSafe(irp->MdlAddress,
                            NormalPagePriority);
        LARGE_INTEGER offset;

        if (system_buffer == NULL)
          {
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        break;
          }

        if (device_extension->vm_disk)
          {
#ifdef _AMD64_
        ULONG_PTR vm_offset =
          io_stack->Parameters.Read.ByteOffset.QuadPart;
#else
        ULONG_PTR vm_offset =
          io_stack->Parameters.Read.ByteOffset.LowPart;
#endif

        RtlCopyMemory(system_buffer,
                  device_extension->image_buffer +
                  vm_offset,
                  io_stack->Parameters.Read.Length);

        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = io_stack->Parameters.Read.Length;

        break;
          }

        offset.QuadPart = io_stack->Parameters.Read.ByteOffset.QuadPart +
          device_extension->image_offset.QuadPart;

        if (device_extension->last_io_data != NULL)
          if ((io_stack->Parameters.Read.ByteOffset.QuadPart >=
           device_extension->last_io_offset) &
          ((io_stack->Parameters.Read.ByteOffset.QuadPart +
            io_stack->Parameters.Read.Length) <=
           (device_extension->last_io_offset +
            device_extension->last_io_length)))
        {
          irp->IoStatus.Status = STATUS_SUCCESS;
          irp->IoStatus.Information = io_stack->Parameters.Read.Length;

          RtlCopyMemory(system_buffer,
                device_extension->last_io_data +
                io_stack->Parameters.Read.ByteOffset.QuadPart -
                device_extension->last_io_offset,
                irp->IoStatus.Information);

          break;
        }
          else if (device_extension->last_io_length <
               io_stack->Parameters.Read.Length)
        {
          ExFreePoolWithTag(device_extension->last_io_data, POOL_TAG);
          device_extension->last_io_data = NULL;
        }

        if (device_extension->last_io_data == NULL)
          device_extension->last_io_data = (PUCHAR)
        ExAllocatePoolWithTag(NonPagedPool,
                      io_stack->Parameters.Read.Length,
                      POOL_TAG);

        device_extension->last_io_offset = 0;
        device_extension->last_io_length = 0;

        if (device_extension->last_io_data == NULL)
          {
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        break;
          }

        if (device_extension->use_proxy)
          {
        irp->IoStatus.Status =
          ImDiskReadProxy(&device_extension->proxy,
                  &irp->IoStatus,
                  &device_extension->terminate_thread,
                  device_extension->last_io_data,
                  io_stack->Parameters.Read.Length,
                  &offset);

        if (!NT_SUCCESS(irp->IoStatus.Status))
          {
            KdPrint(("ImDisk: Read failed on device.\n"/*,
                 device_extension->device_number*/));

            // If indicating that proxy connection died we can do
            // nothing else but remove this device.
            // if (irp->IoStatus.Status == STATUS_CONNECTION_RESET)
            ImDiskRemoveVirtualDisk(device_object);

            irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;
            irp->IoStatus.Information = 0;
          }
          }
        else
          irp->IoStatus.Status =
        ZwReadFile(device_extension->file_handle,
               NULL,
               NULL,
               NULL,
               &irp->IoStatus,
               device_extension->last_io_data,
               io_stack->Parameters.Read.Length,
               &offset,
               NULL);

        RtlCopyMemory(system_buffer, device_extension->last_io_data,
              irp->IoStatus.Information);

        if (NT_SUCCESS(irp->IoStatus.Status))
          {
        device_extension->last_io_offset =
          io_stack->Parameters.Read.ByteOffset.QuadPart;
        device_extension->last_io_length =
          io_stack->Parameters.Read.Length;
          }
        else
          {
        ExFreePoolWithTag(device_extension->last_io_data, POOL_TAG);
        device_extension->last_io_data = NULL;
          }

        break;
      }

    case IRP_MJ_WRITE:
      {
        PUCHAR system_buffer =
          (PUCHAR) MmGetSystemAddressForMdlSafe(irp->MdlAddress,
                            NormalPagePriority);
        LARGE_INTEGER offset;

        if (system_buffer == NULL)
          {
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        break;
          }

        device_extension->image_modified = TRUE;

        if (device_extension->vm_disk)
          {
#ifdef _AMD64_
        ULONG_PTR vm_offset =
          io_stack->Parameters.Write.ByteOffset.QuadPart;
#else
        ULONG_PTR vm_offset =
          io_stack->Parameters.Write.ByteOffset.LowPart;
#endif

        RtlCopyMemory(device_extension->image_buffer +
                  vm_offset,
                  system_buffer,
                  io_stack->Parameters.Write.Length);

        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = io_stack->Parameters.Write.Length;

        break;
          }

        offset.QuadPart = io_stack->Parameters.Write.ByteOffset.QuadPart +
          device_extension->image_offset.QuadPart;

        if (device_extension->last_io_data != NULL)
          if (device_extension->last_io_length <
          io_stack->Parameters.Write.Length)
        {
          ExFreePoolWithTag(device_extension->last_io_data, POOL_TAG);
          device_extension->last_io_data = NULL;
        }

        device_extension->last_io_offset = 0;
        device_extension->last_io_length = 0;

        if (device_extension->last_io_data == NULL)
          device_extension->last_io_data = (PUCHAR)
        ExAllocatePoolWithTag(NonPagedPool,
                      io_stack->Parameters.Write.Length,
                      POOL_TAG);

        if (device_extension->last_io_data == NULL)
          {
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        break;
          }

        RtlCopyMemory(device_extension->last_io_data, system_buffer,
              io_stack->Parameters.Write.Length);

        if (device_extension->use_proxy)
          {
        irp->IoStatus.Status =
          ImDiskWriteProxy(&device_extension->proxy,
                   &irp->IoStatus,
                   &device_extension->terminate_thread,
                   device_extension->last_io_data,
                   io_stack->Parameters.Write.Length,
                   &offset);

        if (!NT_SUCCESS(irp->IoStatus.Status))
          {
            KdPrint(("ImDisk: Write failed on device.\n"/*,
                 device_extension->device_number*/));

            // If indicating that proxy connection died we can do
            // nothing else but remove this device.
            if (irp->IoStatus.Status == STATUS_CONNECTION_RESET)
              ImDiskRemoveVirtualDisk(device_object);

            irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;
            irp->IoStatus.Information = 0;
          }
          }
        else
          {
        BOOLEAN set_zero_data = FALSE;
        if (device_extension->use_set_zero_data)
          {
            PULONGLONG ptr;
            for (set_zero_data = TRUE,
               ptr = (PULONGLONG) device_extension->last_io_data;
             ptr < (PULONGLONG)
               (device_extension->last_io_data +
                io_stack->Parameters.Write.Length);
             ptr++)
              if (*ptr != 0)
            {
              set_zero_data = FALSE;
              break;
            }
          }

        if (set_zero_data)
          {
            FILE_ZERO_DATA_INFORMATION zero_data;
            zero_data.FileOffset = offset;
            zero_data.BeyondFinalZero.QuadPart = offset.QuadPart +
              io_stack->Parameters.Write.Length;

            irp->IoStatus.Status =
              ZwFsControlFile(device_extension->file_handle,
                      NULL,
                      NULL,
                      NULL,
                      &irp->IoStatus,
                      FSCTL_SET_ZERO_DATA,
                      &zero_data,
                      sizeof(zero_data),
                      NULL,
                      0);

            if (NT_SUCCESS(irp->IoStatus.Status))
              {
            KdPrint2(("ImDisk: Zero block set.\n"));
            irp->IoStatus.Information =
              io_stack->Parameters.Write.Length;
              }
            else
              {
            KdPrint(("ImDisk: Volume does not support "
                 "FSCTL_SET_ZERO_DATA: %X\n",
                 irp->IoStatus.Status));

            irp->IoStatus.Information = 0;
            set_zero_data = FALSE;
            device_extension->use_set_zero_data = FALSE;
              }
          }

        if (!set_zero_data)
          irp->IoStatus.Status =
            ZwWriteFile(device_extension->file_handle,
                NULL,
                NULL,
                NULL,
                &irp->IoStatus,
                device_extension->last_io_data,
                io_stack->Parameters.Write.Length,
                &offset,
                NULL);
          }

        if (NT_SUCCESS(irp->IoStatus.Status))
          {
        device_extension->last_io_offset =
          io_stack->Parameters.Write.ByteOffset.QuadPart;
        device_extension->last_io_length =
          io_stack->Parameters.Write.Length;
          }
        else
          {
        ExFreePoolWithTag(device_extension->last_io_data, POOL_TAG);
        device_extension->last_io_data = NULL;
          }

        break;
      }

    case IRP_MJ_DEVICE_CONTROL:
      switch (io_stack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_DISK_CHECK_VERIFY:
        //case IOCTL_CDROM_CHECK_VERIFY:
        case IOCTL_STORAGE_CHECK_VERIFY:
        case IOCTL_STORAGE_CHECK_VERIFY2:
          {
        PUCHAR buffer;

        buffer = (PUCHAR)
          ExAllocatePoolWithTag(NonPagedPool, 1, POOL_TAG);

        if (buffer == NULL)
          {
            irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            irp->IoStatus.Information = 0;
            break;
          }

        if (device_extension->use_proxy)
          irp->IoStatus.Status =
            ImDiskReadProxy(&device_extension->proxy,
                    &irp->IoStatus,
                    &device_extension->terminate_thread,
                    buffer,
                    0,
                    &device_extension->image_offset);
        else
          irp->IoStatus.Status =
            ZwReadFile(device_extension->file_handle,
                   NULL,
                   NULL,
                   NULL,
                   &irp->IoStatus,
                   buffer,
                   0,
                   &device_extension->image_offset,
                   NULL);

        ExFreePoolWithTag(buffer, POOL_TAG);

        if (!NT_SUCCESS(irp->IoStatus.Status))
          {
            KdPrint(("ImDisk: Verify failed on device.\n"/*,
                 device_extension->device_number*/));

            // If indicating that proxy connection died we can do
            // nothing else but remove this device.
            if (irp->IoStatus.Status == STATUS_CONNECTION_RESET)
              ImDiskRemoveVirtualDisk(device_object);

            irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;
            irp->IoStatus.Information = 0;
            break;
          }

        KdPrint(("ImDisk: Verify ok on device.\n"/*,
             device_extension->device_number*/));

        if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(ULONG))
          irp->IoStatus.Information = 0;
        else
          {
            *(PULONG) irp->AssociatedIrp.SystemBuffer =
              device_extension->media_change_count;

            irp->IoStatus.Information = sizeof(ULONG);
          }

        irp->IoStatus.Status = STATUS_SUCCESS;
        break;
          }

        case IOCTL_DISK_GROW_PARTITION:
          {
        NTSTATUS status;
        FILE_END_OF_FILE_INFORMATION new_size;
        FILE_STANDARD_INFORMATION file_standard_information;

        new_size.EndOfFile.QuadPart =
          device_extension->disk_geometry.Cylinders.QuadPart +
          ((PDISK_GROW_PARTITION) irp->AssociatedIrp.SystemBuffer)->
          BytesToGrow.QuadPart;

        if (device_extension->vm_disk)
          {
            PVOID new_image_buffer = NULL;
            SIZE_T free_size = 0;
#ifdef _AMD64_
            ULONG_PTR old_size =
              device_extension->disk_geometry.Cylinders.QuadPart;
            SIZE_T max_size = new_size.EndOfFile.QuadPart;
#else
            ULONG_PTR old_size =
              device_extension->disk_geometry.Cylinders.LowPart;
            SIZE_T max_size = new_size.EndOfFile.LowPart;

            // A vm type disk cannot be extened to a larger size than
            // 2 GB.
            if (new_size.EndOfFile.QuadPart & 0xFFFFFFFF80000000)
              {
            irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            irp->IoStatus.Information = 0;
            break;
              }
#endif //_AMD64_

            status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                             &new_image_buffer,
                             0,
                             &max_size,
                             MEM_COMMIT,
                             PAGE_READWRITE);

            if (!NT_SUCCESS(status))
              {
            status = STATUS_NO_MEMORY;
            irp->IoStatus.Status = status;
            irp->IoStatus.Information = 0;
            break;
              }

            RtlCopyMemory(new_image_buffer,
                  device_extension->image_buffer,
                  old_size);

            ZwFreeVirtualMemory(NtCurrentProcess(),
                    &device_extension->image_buffer,
                    &free_size,
                    MEM_RELEASE);

            device_extension->image_buffer = new_image_buffer;
            device_extension->disk_geometry.Cylinders =
              new_size.EndOfFile;

            irp->IoStatus.Information = 0;
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
          }

        // For proxy-type disks the new size is just accepted and
        // that's it.
        if (device_extension->use_proxy)
          {
            device_extension->disk_geometry.Cylinders =
              new_size.EndOfFile;

            irp->IoStatus.Information = 0;
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
          }

        // Image file backed disks left to do.

        // For disks with offset, refuse to extend size. Otherwise we
        // could break compatibility with the header data we have
        // skipped and we don't know about.
        if (device_extension->image_offset.QuadPart != 0)
          {
            irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            irp->IoStatus.Information = 0;
            break;
          }

        status =
          ZwQueryInformationFile(device_extension->file_handle,
                     &irp->IoStatus,
                     &file_standard_information,
                     sizeof file_standard_information,
                     FileStandardInformation);

        if (!NT_SUCCESS(status))
          {
            irp->IoStatus.Status = status;
            irp->IoStatus.Information = 0;
            break;
          }

        if (file_standard_information.EndOfFile.QuadPart >=
            new_size.EndOfFile.QuadPart)
          {
            device_extension->disk_geometry.Cylinders =
              new_size.EndOfFile;

            irp->IoStatus.Information = 0;
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
          }

        // For other, fixed file-backed disks we need to adjust the
        // physical filesize.

        status = ZwSetInformationFile(device_extension->file_handle,
                          &irp->IoStatus,
                          &new_size,
                          sizeof new_size,
                          FileEndOfFileInformation);

        if (NT_SUCCESS(status))
          device_extension->disk_geometry.Cylinders =
            new_size.EndOfFile;

        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = status;
        break;
          }

        default:
          irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
        }
      break;

    default:
      irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
    }

      IoCompleteRequest(irp,
            NT_SUCCESS(irp->IoStatus.Status) ?
            IO_DISK_INCREMENT : IO_NO_INCREMENT);
    }
}

//
// Reads in a loop up to "Length" or until eof reached.
//
NTSTATUS
ImDiskSafeReadFile(IN HANDLE FileHandle,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           OUT PVOID Buffer,
           IN SIZE_T Length,
           IN PLARGE_INTEGER Offset)
{
  NTSTATUS status;
  SIZE_T LengthDone = 0;

  PAGED_CODE();

  ASSERT(FileHandle != NULL);
  ASSERT(IoStatusBlock != NULL);
  ASSERT(Buffer != NULL);

  while (LengthDone < Length)
    {
      SIZE_T LongRequestLength = Length - LengthDone;
      ULONG RequestLength;
      if (LongRequestLength > 0x0000000080000000)
    RequestLength = 0x80000000;
      else
    RequestLength = (ULONG) LongRequestLength;

      for (;;)
    {
      LARGE_INTEGER RequestOffset;
      PUCHAR InterBuffer = ExAllocatePoolWithTag(PagedPool,
                             RequestLength,
                             POOL_TAG);

      if (InterBuffer == NULL)
        {
          KdPrint(("ImDisk: Insufficient paged pool to allocate "
               "intermediate buffer for ImDiskSafeReadFile() "
               "(%u bytes).\n", RequestLength));

          RequestLength >>= 2;
          continue;
        }

      RequestOffset.QuadPart = Offset->QuadPart + LengthDone;

      status = ZwReadFile(FileHandle,
                  NULL,
                  NULL,
                  NULL,
                  IoStatusBlock,
                  InterBuffer,
                  RequestLength,
                  &RequestOffset,
                  NULL);

      if ((status == STATUS_INSUFFICIENT_RESOURCES) |
          (status == STATUS_INVALID_BUFFER_SIZE) |
          (status == STATUS_INVALID_PARAMETER))
        {
          ExFreePoolWithTag(InterBuffer, POOL_TAG);

          RequestLength >>= 2;
          continue;
        }

      if (!NT_SUCCESS(status))
        {
          ExFreePoolWithTag(InterBuffer, POOL_TAG);
          break;
        }

      RtlCopyMemory((PUCHAR) Buffer + LengthDone, InterBuffer,
            IoStatusBlock->Information);

      ExFreePoolWithTag(InterBuffer, POOL_TAG);
      break;
    }

      if (!NT_SUCCESS(status))
    {
      IoStatusBlock->Status = status;
      IoStatusBlock->Information = LengthDone;
      return IoStatusBlock->Status;
    }

      if (IoStatusBlock->Information == 0)
    {
      IoStatusBlock->Status = STATUS_CONNECTION_RESET;
      IoStatusBlock->Information = LengthDone;
      return IoStatusBlock->Status;
    }

      LengthDone += IoStatusBlock->Information;
    }

  IoStatusBlock->Status = STATUS_SUCCESS;
  IoStatusBlock->Information = LengthDone;
  return IoStatusBlock->Status;
}

NTSTATUS
ImDiskSafeIOStream(IN PFILE_OBJECT FileObject,
           IN UCHAR MajorFunction,
           IN OUT PIO_STATUS_BLOCK IoStatusBlock,
           IN PKEVENT CancelEvent,
           OUT PVOID Buffer,
           IN ULONG Length)
{
  NTSTATUS status;
  ULONG length_done = 0;
  KEVENT io_complete_event;
  PIO_STACK_LOCATION io_stack;
  LARGE_INTEGER offset = { 0 };
  PKEVENT wait_object[] = {
    &io_complete_event,
    CancelEvent
  };
  ULONG number_of_wait_objects = CancelEvent != NULL ? 2 : 1;

  PAGED_CODE();

  KdPrint2(("ImDiskSafeIOStream: FileObject=%#x, MajorFunction=%#x, "
        "IoStatusBlock=%#x, Buffer=%#x, Length=%#x.\n",
        FileObject, MajorFunction, IoStatusBlock, Buffer, Length));

  ASSERT(FileObject != NULL);
  ASSERT(IoStatusBlock != NULL);
  ASSERT(Buffer != NULL);

  KeInitializeEvent(&io_complete_event,
            NotificationEvent,
            FALSE);

  while (length_done < Length)
    {
      ULONG RequestLength = Length - length_done;

      do
    {
      PIRP irp;

      KdPrint2(("ImDiskSafeIOStream: Building IRP...\n"));

      irp = IoBuildSynchronousFsdRequest(MajorFunction,
                         FileObject->DeviceObject,
                         (PUCHAR) Buffer + length_done,
                         RequestLength,
                         &offset,
                         &io_complete_event,
                         IoStatusBlock);

      if (irp == NULL)
        {
          KdPrint(("ImDiskSafeIOStream: Error building IRP.\n"));

          IoStatusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;
          IoStatusBlock->Information = length_done;
          return IoStatusBlock->Status;
        }

      KdPrint2(("ImDiskSafeIOStream: Built IRP=%#x.\n", irp));

      io_stack = IoGetNextIrpStackLocation(irp);
      io_stack->FileObject = FileObject;
      io_stack->DeviceObject = FileObject->DeviceObject;

      KdPrint2(("ImDiskSafeIOStream: MajorFunction=%#x, Length=%#x\n",
            io_stack->MajorFunction,
            io_stack->Parameters.Read.Length));

      KeResetEvent(&io_complete_event);

      status = IoCallDriver(io_stack->FileObject->DeviceObject, irp);

      if (status == STATUS_PENDING)
        {
          status = KeWaitForMultipleObjects(number_of_wait_objects,
                        wait_object,
                        WaitAny,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL,
                        NULL);

          if (KeReadStateEvent(&io_complete_event) == 0)
        {
          IoCancelIrp(irp);
          KeWaitForSingleObject(&io_complete_event,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);
        }
        }
      else if (!NT_SUCCESS(status))
        break;

      status = IoStatusBlock->Status;

      KdPrint2(("ImDiskSafeIOStream: IRP %#x completed. Status=%#x.\n",
            irp, IoStatusBlock->Status));

      RequestLength >>= 1;
    }
      while ((status == STATUS_INVALID_BUFFER_SIZE) |
         (status == STATUS_INVALID_PARAMETER));

      if (!NT_SUCCESS(status))
    {
      KdPrint2(("ImDiskSafeIOStream: I/O failed. Status=%#x.\n", status));

      IoStatusBlock->Status = status;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

      KdPrint2(("ImDiskSafeIOStream: I/O done. Status=%#x. Length=%#x\n",
        status, IoStatusBlock->Information));

      if (IoStatusBlock->Information == 0)
    {
      IoStatusBlock->Status = STATUS_CONNECTION_RESET;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

      length_done += (ULONG) IoStatusBlock->Information;
    }

  KdPrint2(("ImDiskSafeIOStream: I/O complete.\n"));

  IoStatusBlock->Status = STATUS_SUCCESS;
  IoStatusBlock->Information = length_done;
  return IoStatusBlock->Status;
}

VOID
ImDiskCloseProxy(IN PPROXY_CONNECTION Proxy)
{
  PAGED_CODE();

  ASSERT(Proxy != NULL);

  switch (Proxy->connection_type)
    {
    case PROXY_CONNECTION_DEVICE:
      if (Proxy->device != NULL)
    ObDereferenceObject(Proxy->device);

      Proxy->device = NULL;
      break;

    case PROXY_CONNECTION_SHM:
      if ((Proxy->request_event != NULL) &
      (Proxy->response_event != NULL) &
      (Proxy->shared_memory != NULL))
    {
      *(ULONGLONG*)Proxy->shared_memory = IMDPROXY_REQ_CLOSE;
      KeSetEvent(Proxy->request_event, (KPRIORITY) 0, FALSE);
    }

      if (Proxy->request_event_handle != NULL)
    {
      ZwClose(Proxy->request_event_handle);
      Proxy->request_event_handle = NULL;
    }

      if (Proxy->response_event_handle != NULL)
    {
      ZwClose(Proxy->response_event_handle);
      Proxy->response_event_handle = NULL;
    }

      if (Proxy->request_event != NULL)
    {
      ObDereferenceObject(Proxy->request_event);
      Proxy->request_event = NULL;
    }

      if (Proxy->response_event != NULL)
    {
      ObDereferenceObject(Proxy->response_event);
      Proxy->response_event = NULL;
    }

      if (Proxy->shared_memory != NULL)
    {
      ZwUnmapViewOfSection(NtCurrentProcess(), Proxy->shared_memory);
      Proxy->shared_memory = NULL;
    }

      break;
    }
}

NTSTATUS
ImDiskCallProxy(IN PPROXY_CONNECTION Proxy,
        OUT PIO_STATUS_BLOCK IoStatusBlock,
        IN PKEVENT CancelEvent OPTIONAL,
        IN PVOID RequestHeader,
        IN ULONG RequestHeaderSize,
        IN PVOID RequestData,
        IN ULONG RequestDataSize,
        IN OUT PVOID ResponseHeader,
        IN ULONG ResponseHeaderSize,
        IN OUT PVOID ResponseData,
        IN ULONG ResponseDataBufferSize,
        IN ULONG *ResponseDataSize)
{
  NTSTATUS status;

  PAGED_CODE();

  ASSERT(Proxy != NULL);

  switch (Proxy->connection_type)
    {
    case PROXY_CONNECTION_DEVICE:
      {
    if (RequestHeaderSize > 0)
      {
        if (CancelEvent != NULL ?
        KeReadStateEvent(CancelEvent) != 0 :
        FALSE)
          {
        KdPrint(("ImDisk Proxy Client: Request cancelled.\n."));

        IoStatusBlock->Status = STATUS_CANCELLED;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }

        status = ImDiskSafeIOStream(Proxy->device,
                    IRP_MJ_WRITE,
                    IoStatusBlock,
                    CancelEvent,
                    RequestHeader,
                    RequestHeaderSize);

        if (!NT_SUCCESS(status))
          {
        KdPrint(("ImDisk Proxy Client: Request header error %#x\n.",
             status));

        IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }
      }

    if (RequestDataSize > 0)
      {
        if (CancelEvent != NULL ?
        KeReadStateEvent(CancelEvent) != 0 :
        FALSE)
          {
        KdPrint(("ImDisk Proxy Client: Request cancelled.\n."));

        IoStatusBlock->Status = STATUS_CANCELLED;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }

        KdPrint2
          (("ImDisk Proxy Client: Sent req. Sending data stream.\n"));

        status = ImDiskSafeIOStream(Proxy->device,
                    IRP_MJ_WRITE,
                    IoStatusBlock,
                    CancelEvent,
                    RequestData,
                    RequestDataSize);

        if (!NT_SUCCESS(status))
          {
        KdPrint(("ImDisk Proxy Client: Data stream send failed. "
             "Sent %u bytes with I/O status %#x.\n",
             IoStatusBlock->Information, IoStatusBlock->Status));

        IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }

        KdPrint2
          (("ImDisk Proxy Client: Data stream of %u bytes sent with I/O "
        "status %#x. Status returned by stream writer is %#x. "
        "Waiting for IMDPROXY_RESP_WRITE.\n",
        IoStatusBlock->Information, IoStatusBlock->Status, status));
      }

    if (ResponseHeaderSize > 0)
      {
        if (CancelEvent != NULL ?
        KeReadStateEvent(CancelEvent) != 0 :
        FALSE)
          {
        KdPrint(("ImDisk Proxy Client: Request cancelled.\n."));

        IoStatusBlock->Status = STATUS_CANCELLED;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }

        status = ImDiskSafeIOStream(Proxy->device,
                    IRP_MJ_READ,
                    IoStatusBlock,
                    CancelEvent,
                    ResponseHeader,
                    ResponseHeaderSize);

        if (!NT_SUCCESS(status))
          {
        KdPrint(("ImDisk Proxy Client: Response header error %#x\n.",
             status));

        IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }
      }

    if (ResponseDataSize != NULL ? *ResponseDataSize > 0 : FALSE)
      {
        if (*ResponseDataSize > ResponseDataBufferSize)
          {
        KdPrint(("ImDisk Proxy Client: Fatal: Request %u bytes, "
             "receiving %u bytes.\n",
             ResponseDataBufferSize, *ResponseDataSize));

        IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }

        if (CancelEvent != NULL ?
        KeReadStateEvent(CancelEvent) != 0 :
        FALSE)
          {
        KdPrint(("ImDisk Proxy Client: Request cancelled.\n."));

        IoStatusBlock->Status = STATUS_CANCELLED;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }

        KdPrint2
          (("ImDisk Proxy Client: Got ok resp. Waiting for data.\n"));

        status = ImDiskSafeIOStream(Proxy->device,
                    IRP_MJ_READ,
                    IoStatusBlock,
                    CancelEvent,
                    ResponseData,
                    *ResponseDataSize);

        if (!NT_SUCCESS(status))
          {
        KdPrint(("ImDisk Proxy Client: Response data error %#x\n.",
             status));

        KdPrint(("ImDisk Proxy Client: Response data %u bytes, "
             "got %u bytes.\n",
             *ResponseDataSize,
             (ULONG) IoStatusBlock->Information));

        IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
          }

        KdPrint2
          (("ImDisk Proxy Client: Received %u byte data stream.\n",
        IoStatusBlock->Information));
      }

    IoStatusBlock->Status = STATUS_SUCCESS;
    if ((RequestDataSize > 0) & (IoStatusBlock->Information == 0))
      IoStatusBlock->Information = RequestDataSize;
    return IoStatusBlock->Status;
      }

    case PROXY_CONNECTION_SHM:
      {
    PKEVENT wait_objects[] = {
      Proxy->response_event,
      CancelEvent
    };

    ULONG number_of_wait_objects = CancelEvent != NULL ? 2 : 1;

    // Some parameter sanity checks
    if ((RequestHeaderSize > IMDPROXY_HEADER_SIZE) |
        (ResponseHeaderSize > IMDPROXY_HEADER_SIZE) |
        ((RequestDataSize + IMDPROXY_HEADER_SIZE) >
         Proxy->shared_memory_size))
      {
        KdPrint(("ImDisk Proxy Client: "
             "Parameter values not supported.\n."));

        IoStatusBlock->Status = STATUS_INVALID_BUFFER_SIZE;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
      }

    IoStatusBlock->Information = 0;

    if (RequestHeaderSize > 0)
      RtlCopyMemory(Proxy->shared_memory,
            RequestHeader,
            RequestHeaderSize);

    if (RequestDataSize > 0)
      RtlCopyMemory(Proxy->shared_memory + IMDPROXY_HEADER_SIZE,
            RequestData,
            RequestDataSize);

    KeSetEvent(Proxy->request_event, (KPRIORITY) 0, TRUE);

    status = KeWaitForMultipleObjects(number_of_wait_objects,
                      wait_objects,
                      WaitAny,
                      Executive,
                      KernelMode,
                      FALSE,
                      NULL,
                      NULL);

    if (status == STATUS_WAIT_1)
      {
        KdPrint(("ImDisk Proxy Client: Incomplete wait %#x.\n.", status));

        IoStatusBlock->Status = STATUS_CANCELLED;
        IoStatusBlock->Information = 0;
        return IoStatusBlock->Status;
      }

    if (ResponseHeaderSize > 0)
      RtlCopyMemory(ResponseHeader,
            Proxy->shared_memory,
            ResponseHeaderSize);

    // If server end requests to send more data than we requested, we
    // treat that as an unrecoverable device error and exit.
    if (ResponseDataSize != NULL ? *ResponseDataSize > 0 : FALSE)
      if ((*ResponseDataSize > ResponseDataBufferSize) |
          ((*ResponseDataSize + IMDPROXY_HEADER_SIZE) >
           Proxy->shared_memory_size))
        {
          KdPrint(("ImDisk Proxy Client: Invalid response size %u.\n.",
               *ResponseDataSize));

          IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
          IoStatusBlock->Information = 0;
          return IoStatusBlock->Status;
        }
      else
        {
          RtlCopyMemory(ResponseData,
                Proxy->shared_memory + IMDPROXY_HEADER_SIZE,
                *ResponseDataSize);

          IoStatusBlock->Information = *ResponseDataSize;
        }

    IoStatusBlock->Status = STATUS_SUCCESS;
    if ((RequestDataSize > 0) & (IoStatusBlock->Information == 0))
      IoStatusBlock->Information = RequestDataSize;
    return IoStatusBlock->Status;
      }

    default:
      return STATUS_DRIVER_INTERNAL_ERROR;
    }
}

///
/// Note that this function when successful replaces the Proxy->device pointer
/// to point to the connected device object instead of the proxy service pipe.
/// This means that the only reference to the proxy service pipe after calling
/// this function is the original handle to the pipe.
///
NTSTATUS
ImDiskConnectProxy(IN OUT PPROXY_CONNECTION Proxy,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           IN PKEVENT CancelEvent OPTIONAL,
           IN ULONG Flags,
           IN PWSTR ConnectionString,
           IN USHORT ConnectionStringLength)
{
  IMDPROXY_CONNECT_REQ connect_req;
  IMDPROXY_CONNECT_RESP connect_resp;
  NTSTATUS status;

  PAGED_CODE();

  ASSERT(Proxy != NULL);
  ASSERT(IoStatusBlock != NULL);
  ASSERT(ConnectionString != NULL);

  if (IMDISK_PROXY_TYPE(Flags) == IMDISK_PROXY_TYPE_SHM)
    {
      OBJECT_ATTRIBUTES object_attributes;
      UNICODE_STRING base_name = { 0 };
      UNICODE_STRING event_name = { 0 };
      base_name.Buffer = ConnectionString;
      base_name.Length = ConnectionStringLength;
      base_name.MaximumLength = ConnectionStringLength;
      event_name.MaximumLength = ConnectionStringLength + 20;
      event_name.Buffer = ExAllocatePoolWithTag(PagedPool,
                        event_name.MaximumLength,
                        POOL_TAG);
      if (event_name.Buffer == NULL)
    {
      status = STATUS_INSUFFICIENT_RESOURCES;

      IoStatusBlock->Status = status;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

      InitializeObjectAttributes(&object_attributes,
                 &event_name,
                 OBJ_CASE_INSENSITIVE,
                 NULL,
                 NULL);

      RtlCopyUnicodeString(&event_name, &base_name);
      RtlAppendUnicodeToString(&event_name, L"_Request");

      status = ZwOpenEvent(&Proxy->request_event_handle,
               EVENT_ALL_ACCESS,
               &object_attributes);

      if (!NT_SUCCESS(status))
    {
      Proxy->request_event_handle = NULL;
      ExFreePoolWithTag(event_name.Buffer, POOL_TAG);

      IoStatusBlock->Status = status;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

      status = ObReferenceObjectByHandle(Proxy->request_event_handle,
                     EVENT_ALL_ACCESS,
                     *ExEventObjectType,
                     KernelMode,
                     &Proxy->request_event,
                     NULL);

      if (!NT_SUCCESS(status))
    {
      Proxy->request_event = NULL;
      ExFreePoolWithTag(event_name.Buffer, POOL_TAG);

      IoStatusBlock->Status = status;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

      RtlCopyUnicodeString(&event_name, &base_name);
      RtlAppendUnicodeToString(&event_name, L"_Response");

      status = ZwOpenEvent(&Proxy->response_event_handle,
               EVENT_ALL_ACCESS,
               &object_attributes);

      if (!NT_SUCCESS(status))
    {
      Proxy->response_event_handle = NULL;
      ExFreePoolWithTag(event_name.Buffer, POOL_TAG);

      IoStatusBlock->Status = status;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

      status = ObReferenceObjectByHandle(Proxy->response_event_handle,
                     EVENT_ALL_ACCESS,
                     *ExEventObjectType,
                     KernelMode,
                     &Proxy->response_event,
                     NULL);

      if (!NT_SUCCESS(status))
    {
      Proxy->response_event = NULL;
      ExFreePoolWithTag(event_name.Buffer, POOL_TAG);

      IoStatusBlock->Status = status;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

      IoStatusBlock->Status = STATUS_SUCCESS;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  connect_req.request_code = IMDPROXY_REQ_CONNECT;
  connect_req.flags = Flags;
  connect_req.length = ConnectionStringLength;

  KdPrint(("ImDisk Proxy Client: Sending IMDPROXY_CONNECT_REQ.\n"));

  status = ImDiskCallProxy(Proxy,
               IoStatusBlock,
               CancelEvent,
               &connect_req,
               sizeof(connect_req),
               ConnectionString,
               ConnectionStringLength,
               &connect_resp,
               sizeof(IMDPROXY_CONNECT_RESP),
               NULL,
               0,
               NULL);

  if (!NT_SUCCESS(status))
    {
      IoStatusBlock->Status = status;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  if (connect_resp.error_code != 0)
    {
      IoStatusBlock->Status = STATUS_CONNECTION_REFUSED;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  // If the proxy gave us a reference to an object to use for direct connection
  // to the server we have to change the active reference to use here.
  if (connect_resp.object_ptr != 0)
    {
      ObDereferenceObject(Proxy->device);
      Proxy->device = (PFILE_OBJECT)(ULONG_PTR) connect_resp.object_ptr;
    }

  KdPrint(("ImDisk Proxy Client: Got ok response IMDPROXY_CONNECT_RESP.\n"));

  IoStatusBlock->Status = STATUS_SUCCESS;
  IoStatusBlock->Information = 0;
  return IoStatusBlock->Status;
}

NTSTATUS
ImDiskQueryInformationProxy(IN PPROXY_CONNECTION Proxy,
                OUT PIO_STATUS_BLOCK IoStatusBlock,
                IN PKEVENT CancelEvent,
                OUT PIMDPROXY_INFO_RESP ProxyInfoResponse,
                IN ULONG ProxyInfoResponseLength)
{
  ULONGLONG proxy_req = IMDPROXY_REQ_INFO;
  NTSTATUS status;

  PAGED_CODE();

  ASSERT(Proxy != NULL);
  ASSERT(IoStatusBlock != NULL);

  if ((ProxyInfoResponse == NULL) |
      (ProxyInfoResponseLength < sizeof(IMDPROXY_INFO_RESP)))
    {
      IoStatusBlock->Status = STATUS_BUFFER_OVERFLOW;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  KdPrint(("ImDisk Proxy Client: Sending IMDPROXY_REQ_INFO.\n"));

  status = ImDiskCallProxy(Proxy,
               IoStatusBlock,
               CancelEvent,
               &proxy_req,
               sizeof(proxy_req),
               NULL,
               0,
               ProxyInfoResponse,
               sizeof(IMDPROXY_INFO_RESP),
               NULL,
               0,
               NULL);

  if (!NT_SUCCESS(status))
    {
      IoStatusBlock->Status = status;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  KdPrint(("ImDisk Proxy Client: Got ok response IMDPROXY_INFO_RESP.\n"));

  if (ProxyInfoResponse->req_alignment - 1 > FILE_512_BYTE_ALIGNMENT)
    {
      KdPrint(("ImDisk IMDPROXY_INFO_RESP: Unsupported sizes. "
           "Got %p-%p size and %p-%p alignment.\n",
           ProxyInfoResponse->file_size,
           ProxyInfoResponse->req_alignment));

      IoStatusBlock->Status = STATUS_INVALID_PARAMETER;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  IoStatusBlock->Status = STATUS_SUCCESS;
  IoStatusBlock->Information = 0;
  return IoStatusBlock->Status;
}

NTSTATUS
ImDiskReadProxy(IN PPROXY_CONNECTION Proxy,
        OUT PIO_STATUS_BLOCK IoStatusBlock,
        IN PKEVENT CancelEvent,
        OUT PVOID Buffer,
        IN ULONG Length,
        IN PLARGE_INTEGER ByteOffset)
{
  IMDPROXY_READ_REQ read_req;
  IMDPROXY_READ_RESP read_resp;
  NTSTATUS status;

  PAGED_CODE();

  ASSERT(Proxy != NULL);
  ASSERT(IoStatusBlock != NULL);
  ASSERT(Buffer != NULL);
  ASSERT(ByteOffset != NULL);

  read_req.request_code = IMDPROXY_REQ_READ;
  read_req.offset = ByteOffset->QuadPart;
  read_req.length = Length;

  KdPrint2(("ImDisk Proxy Client: IMDPROXY_REQ_READ %u bytes at %u.\n",
        (ULONG) read_req.length, (ULONG) read_req.offset));

  status = ImDiskCallProxy(Proxy,
               IoStatusBlock,
               CancelEvent,
               &read_req,
               sizeof(read_req),
               NULL,
               0,
               &read_resp,
               sizeof(read_resp),
               Buffer,
               Length,
               (PULONG) &read_resp.length);

  if (!NT_SUCCESS(status))
    {
      IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  if (read_resp.errorno != 0)
    {
      KdPrint(("ImDisk Proxy Client: Server returned error %p-%p.\n",
           read_resp.errorno));
      IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  return status;
}

NTSTATUS
ImDiskWriteProxy(IN PPROXY_CONNECTION Proxy,
         OUT PIO_STATUS_BLOCK IoStatusBlock,
         IN PKEVENT CancelEvent,
         IN PVOID Buffer,
         IN ULONG Length,
         IN PLARGE_INTEGER ByteOffset)
{
  IMDPROXY_READ_REQ write_req;
  IMDPROXY_READ_RESP write_resp;
  NTSTATUS status;

  PAGED_CODE();

  ASSERT(Proxy != NULL);
  ASSERT(IoStatusBlock != NULL);
  ASSERT(Buffer != NULL);
  ASSERT(ByteOffset != NULL);

  write_req.request_code = IMDPROXY_REQ_WRITE;
  write_req.offset = ByteOffset->QuadPart;
  write_req.length = Length;

  KdPrint2(("ImDisk Proxy Client: IMDPROXY_REQ_WRITE %u bytes at %u.\n",
        (ULONG) write_req.length, (ULONG) write_req.offset));

  status = ImDiskCallProxy(Proxy,
               IoStatusBlock,
               CancelEvent,
               &write_req,
               sizeof(write_req),
               Buffer,
               (ULONG) write_req.length,
               &write_resp,
               sizeof(write_resp),
               NULL,
               0,
               NULL);

  if (!NT_SUCCESS(status))
    {
      IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  if (write_resp.errorno != 0)
    {
      KdPrint(("ImDisk Proxy Client: Server returned error %p-%p.\n",
           write_resp.errorno));
      IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  if (write_resp.length != Length)
    {
      KdPrint(("ImDisk Proxy Client: IMDPROXY_REQ_WRITE %u bytes, "
           "IMDPROXY_RESP_WRITE %u bytes.\n",
           Length, (ULONG) write_resp.length));
      IoStatusBlock->Status = STATUS_IO_DEVICE_ERROR;
      IoStatusBlock->Information = 0;
      return IoStatusBlock->Status;
    }

  KdPrint2(("ImDisk Proxy Client: Got ok response. "
        "Resetting IoStatusBlock fields.\n"));

  IoStatusBlock->Status = STATUS_SUCCESS;
  IoStatusBlock->Information = Length;
  return IoStatusBlock->Status;
}
