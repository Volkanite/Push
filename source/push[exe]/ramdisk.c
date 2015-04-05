#include <sl.h>
#include <push.h>
#include <ring0.h>

#include "ramdisk.h"


PFORMATEX   FormatEx;

#define ERROR_NOT_ENOUGH_MEMORY 0x8
//----------------------------------------------------------------------
//
// FormatExCallback
//
// The file system library will call us back with commands that we
// can interpret. If we wanted to halt the chkdsk we could return FALSE.
//
//----------------------------------------------------------------------
BOOLEAN __stdcall FormatExCallback( CALLBACKCOMMAND Command, DWORD Modifier, VOID* Argument )
{
    return TRUE;
}


WCHAR
__stdcall
FindFreeDriveLetter()
{
  DWORD     dwLogicalDrives;
  WCHAR search;


  dwLogicalDrives = GetLogicalDrives();

  for (search = L'D'; search <= L'Z'; search++)
    if ((dwLogicalDrives & (1 << (search - L'A'))) == 0)
      return search;

  return 0;
}


VOID*
__stdcall
OpenDeviceByName( UNICODE_STRING *FileName )
{
  LONG status;
  VOID* handle;
  OBJECT_ATTRIBUTES objectAttributes;
  IO_STATUS_BLOCK   isb;

 objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
 objectAttributes.RootDirectory = NULL;
 objectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
 objectAttributes.ObjectName = FileName;
 objectAttributes.SecurityDescriptor = NULL;
 objectAttributes.SecurityQualityOfService = NULL;

  /*status = NtOpenFile(
            &handle,
            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
            &objAttrib,
            &io_status,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );*/

 status = NtCreateFile(
            &handle,
            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
            &objectAttributes,
            &isb,
            0,
            NULL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );

  if (!NT_SUCCESS(status))
    {
      SetLastError(RtlNtStatusToDosError(status));

      return INVALID_HANDLE_VALUE;
    }

  return handle;
}


VOID
CreateRamDisk( UINT32 Size, CHAR DriveLetter )
{
    RAMDISK_CREATE_DATA createData = { 0 };
    WCHAR mountPoint[]  = L" :";
    NTSTATUS status;

    createData.DriveLetter = DriveLetter;
    createData.DiskGeometry.Cylinders.QuadPart = Size;

    status = R0CreateRamDisk(&createData);

    if (!NT_SUCCESS(status))
    {
        //if (RtlNtStatusToDosError(status) == ERROR_NOT_ENOUGH_MEMORY)
        if (status = STATUS_SECTION_TOO_BIG)
        {
            MessageBoxW(
                NULL,
                L"Not enough memory to create ram disk.\n"
                L"Please upgrade RAM.",
                L"Push",
                NULL
                );

            return;
        }
    }

    mountPoint[0] = DriveLetter;

    DefineDosDeviceW(
        DDD_RAW_TARGET_PATH,
        mountPoint,
        PUSH_RAMDISK_DEVICE_NAME
        );
}


VOID*
OpenRamDisk()
{
    UNICODE_STRING  fileName;

    SlInitUnicodeString(
        &fileName,
        PUSH_RAMDISK_DEVICE_NAME
        );

    return OpenDeviceByName(
                &fileName
                );
}


VOID
FormatRamDisk()
{
    WCHAR mountPoint[]    = L" :";
    //WCHAR cDriveLetter  = 0;
    VOID *deviceHandle       = 0;
    RAMDISK_CREATE_DATA createData = { 0 };

    deviceHandle = OpenRamDisk();

    if (deviceHandle == INVALID_HANDLE_VALUE)
        return;

    //cDriveLetter = GetRamDiskDriveLetter(deviceHandle);
    R0QueryRamDisk(deviceHandle, &createData);

    NtClose(deviceHandle);

    /*if(cDriveLetter == 0)
        return;*/

    if (createData.DriveLetter == NULL)
        //could not get a drive letter, return.
        return;


    //szMountPoint[0] = cDriveLetter;
    mountPoint[0] = createData.DriveLetter;

    /*FormatEx = (PFORMATEX) GetProcAddress(
                            PushLoadLibrary(L"fmifs.dll"),
                            "FormatEx"
                            );*/

    FormatEx = (PFORMATEX) GetProcAddress(
                            SlLoadLibrary(L"fmifs.dll"),
                            "FormatEx"
                            );

    FormatEx(
        mountPoint,
        FMIFS_HARDDISK,
        L"NTFS",
        L"RAM Disk",
        TRUE,
        0,
        FormatExCallback
        );
}


VOID
RemoveRamDisk()
{
    WCHAR mountPoint[] = L" :"/*, cDriveLetter = 0*/;
    VOID *deviceHandle       = 0;
    IO_STATUS_BLOCK isb;
    RAMDISK_CREATE_DATA createData = { 0 };

    deviceHandle = OpenRamDisk();

    if (deviceHandle == INVALID_HANDLE_VALUE)
        return;

    /*cDriveLetter = GetRamDiskDriveLetter(
                    deviceHandle
                    );*/
    R0QueryRamDisk(deviceHandle, &createData);


    /*if(cDriveLetter == 0)
        return;*/
    if (createData.DriveLetter == NULL)
        //could not get a drive letter, return.
        return;

    //szMountPoint[0] = cDriveLetter;
    mountPoint[0] = createData.DriveLetter;

    FlushFileBuffers(deviceHandle);

    NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_LOCK_VOLUME,
        NULL,
        NULL,
        NULL,
        NULL
        );

    NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_DISMOUNT_VOLUME,
        NULL,
        NULL,
        NULL,
        NULL
        );

    NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_STORAGE_EJECT_MEDIA,
        NULL,
        NULL,
        NULL,
        NULL
        );

    NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_UNLOCK_VOLUME,
        NULL,
        NULL,
        NULL,
        NULL
        );

    //CloseHandle(DeviceHandle);
    NtClose(deviceHandle);

    DefineDosDeviceW(DDD_REMOVE_DEFINITION,
                     mountPoint,
                     NULL);
}


