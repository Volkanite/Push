#include <ntifs.h>

#include <mountmgr.h>
#include <mountdev.h>
#include <ntdddisk.h>
#include <ntddvol.h>
//#include <sltypes.h>
//#include <slnt.h>
#include <initguid.h>
//#include <slddktypes.h>
#include <stdio.h>


#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#if DEBUG_LEVEL >= 2
#define KdPrint2(x) DbgPrint x
#else
#define KdPrint2(x)
#endif

#if DEBUG_LEVEL >= 1
#undef KdPrint
#define KdPrint(x)  DbgPrint x
#define ImDiskLogError(x) ImDiskLogDbgError x
#else
#define ImDiskLogError(x)
#endif

#include "ntkmapi.h"
#include "../pushbase.h"
#include "../ntkapi.h"
#include "imdproxy.h"

#define POOL_TAG                         'iDmI'

#define IMDISK_DEFAULT_LOAD_DEVICES      0
#define IMDISK_DEFAULT_MAX_DEVICES       64


//Mapped memory information list
typedef struct tagMAPINFO
{
    SINGLE_LIST_ENTRY   link;
    PMDL                pMdl;   //allocated mdl
    PVOID               pvk;    //kernel mode virtual address
    PVOID               pvu;    //user mode virtual address
    ULONG               memSize;//memory size in bytes
} MAPINFO, *PMAPINFO;

SINGLE_LIST_ENTRY lstMapInfo;   //mapped memory information

// This structure is used when a new device is about to be created. It is sent
// to the created device dispatch thread which also creates the device object.
typedef struct _DEVICE_THREAD_DATA
{
  PDRIVER_OBJECT driver_object;
  RAMDISK_CREATE_DATA* create_data;
  PETHREAD client_thread;   // The client thread that device should impersonate
  KEVENT created_event;     // Set when device is created (or creation failed)
  BOOLEAN caller_waiting;   // If there is a caller waiting to free this data
  NTSTATUS status;          // Set after device creation attempt
} DEVICE_THREAD_DATA, *PDEVICE_THREAD_DATA;

typedef struct _PROXY_CONNECTION
{
  enum
    {
      PROXY_CONNECTION_DEVICE,
      PROXY_CONNECTION_SHM
    } connection_type;       // Connection type

  union
  {
    // Valid if connection_type is PROXY_CONNECTION_DEVICE
    PFILE_OBJECT device;     // Pointer to proxy communication object

    // Valid if connection_type is PROXY_CONNECTION_SHM
    struct
    {
      HANDLE request_event_handle;
      PKEVENT request_event;
      HANDLE response_event_handle;
      PKEVENT response_event;
      PUCHAR shared_memory;
      ULONG_PTR shared_memory_size;
    };
  };
} PROXY_CONNECTION, *PPROXY_CONNECTION;

typedef ULONG_PTR KSPIN_LOCK;
typedef struct _DEVICE_EXTENSION
{
  LIST_ENTRY list_head;
  KSPIN_LOCK list_lock;
  KEVENT request_event;
  KEVENT terminate_thread;

  //ULONG device_number;

  HANDLE file_handle;          // For file or proxy type
  PUCHAR image_buffer;         // For vm type
  PROXY_CONNECTION proxy;      // Proxy connection data
  UNICODE_STRING file_name;    // Name of image file, if any
  WCHAR drive_letter;          // Drive letter if maintained by the driver
  DISK_GEOMETRY disk_geometry; // Virtual C/H/S geometry (Cylinders=Total size)
  LARGE_INTEGER image_offset;  // Offset in bytes in the image file
  ULONG media_change_count;
  BOOLEAN read_only;
  BOOLEAN vm_disk;             // TRUE if this device is a virtual memory disk
  BOOLEAN use_proxy;           // TRUE if this device uses proxy device for I/O
  BOOLEAN image_modified;      // TRUE if this device has been written to
  LONG special_file_count;     // Number of swapfiles/hiberfiles on device
  BOOLEAN use_set_zero_data;   // TRUE if FSCTL_SET_ZERO_DATA is used to write
                               // all zeroes blocks

  PKTHREAD device_thread;      // Pointer to the dispatch thread object

  PUCHAR last_io_data;
  LONGLONG last_io_offset;
  ULONG last_io_length;

    struct _PROCESS_EVENT
    {
      PROCESSID ProcessID;

    } ProcessEvent;

    THREAD_CALLBACK_INFO threadCallbackInfo;

    struct _IMAGE_EVENT
    {
        PROCESSID  processID;
        WCHAR   imageName[260];

    } imageEvent;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//forward function declaration
NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchIoCtl(IN PDEVICE_OBJECT fdo, IN PIRP irp);
VOID PushUnload(IN PDRIVER_OBJECT dro);

// Prototypes for functions defined in this driver

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
        IN PUNICODE_STRING RegistryPath);

VOID
RdUnload(IN PDRIVER_OBJECT DriverObject);

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
          IN PWCHAR Message);

NTSTATUS
ImDiskAddVirtualDisk(IN PDRIVER_OBJECT DriverObject,
             IN OUT RAMDISK_CREATE_DATA* CreateData,
             IN PETHREAD ClientThread);

NTSTATUS
ImDiskAddVirtualDiskAfterInitialization(IN PDRIVER_OBJECT DriverObject,
                    IN HANDLE ParameterKey,
                    IN ULONG DeviceNumber);

NTSTATUS
ImDiskCreateDriveLetter(IN WCHAR DriveLetter);

NTSTATUS
ImDiskRemoveDriveLetter(IN WCHAR DriveLetter);

VOID
ImDiskRemoveVirtualDisk(IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
ImDiskCreateClose(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);

NTSTATUS
ImDiskReadWrite(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp);

NTSTATUS
ImDiskDispatchPnP(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);

VOID
ImDiskDeviceThread(IN PVOID Context);

NTSTATUS
ImDiskConnectProxy(IN OUT PPROXY_CONNECTION Proxy,
           IN OUT PIO_STATUS_BLOCK IoStatusBlock,
           IN PKEVENT CancelEvent OPTIONAL,
           IN ULONG Flags,
           IN PWSTR ConnectionString,
           IN USHORT ConnectionStringLength);

VOID
ImDiskCloseProxy(IN PPROXY_CONNECTION Proxy);

NTSTATUS
ImDiskQueryInformationProxy(IN PPROXY_CONNECTION Proxy,
                IN OUT PIO_STATUS_BLOCK IoStatusBlock,
                IN PKEVENT CancelEvent OPTIONAL,
                OUT PIMDPROXY_INFO_RESP ProxyInfoResponse,
                IN ULONG ProxyInfoResponseLength);

NTSTATUS
ImDiskReadProxy(IN PPROXY_CONNECTION Proxy,
        IN OUT PIO_STATUS_BLOCK IoStatusBlock,
        IN PKEVENT CancelEvent OPTIONAL,
        OUT PVOID Buffer,
        IN ULONG Length,
        IN PLARGE_INTEGER ByteOffset);

NTSTATUS
ImDiskWriteProxy(IN PPROXY_CONNECTION Proxy,
         IN OUT PIO_STATUS_BLOCK IoStatusBlock,
         IN PKEVENT CancelEvent OPTIONAL,
         OUT PVOID Buffer,
         IN ULONG Length,
         IN PLARGE_INTEGER ByteOffset);

//
// Reads in a loop up to "Length" or until eof reached.
//
NTSTATUS
ImDiskSafeReadFile(IN HANDLE FileHandle,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           OUT PVOID Buffer,
           IN SIZE_T Length,
           IN PLARGE_INTEGER Offset);

//
// IOCTL handler for setting the callback
//
NTSTATUS ToggleProcessMonitoring(PIRP Irp);

//
// Process function callback
//
VOID ProcessCallback(
    IN HANDLE  hParentId,
    IN HANDLE  hProcessId,
    IN BOOLEAN bCreate
    );

char szCacheName[260];

//
// Pointer to the controller device object.
//
extern PDEVICE_OBJECT g_DeviceObject;

extern PDEVICE_OBJECT g_pDiskDeviceObject;

//
// Allocation bitmap with currently cnfigured device numbers.
//
//extern volatile ULONGLONG DeviceList;

//
// Max number of devices that can be dynamically created by IOCTL calls
// to the control device. Note that because the device number allocation is
// stored in a 64-bit bitfield, this number can be max 64.
//
ULONG MaxDevices;
// For hard drive partition-style devices
#define SECTOR_SIZE_HDD                  512
#define HEAD_SIZE_HDD                    63

//
//      Indexes for the following DISK_GEOMETRY table.
//
enum {
  // 3.5" UHD
  MEDIA_TYPE_240M,
  MEDIA_TYPE_120M,
  // 3.5"
  MEDIA_TYPE_2880K,
  MEDIA_TYPE_1722K,
  MEDIA_TYPE_1680K,
  MEDIA_TYPE_1440K,
  MEDIA_TYPE_820K,
  MEDIA_TYPE_720K,
  // 5.12"
  MEDIA_TYPE_1200K,
  MEDIA_TYPE_640K,
  MEDIA_TYPE_360K,
  MEDIA_TYPE_320K,
  MEDIA_TYPE_180K,
  MEDIA_TYPE_160K
};

extern DISK_GEOMETRY media_table[];

//
//  TOC Data Track returned for virtual CD/DVD
//
#define TOC_DATA_TRACK                   0x04

//
//  Fill character for formatting virtual floppy media
//
#define MEDIA_FORMAT_FILL_DATA  0xf6

// This structure is used when a new device is about to be created. It is sent
// to the created device dispatch thread which also creates the device object.


//
// An array of boolean values for each drive letter where TRUE means a
// drive letter disallowed for use by ImDisk devices.
//
extern BOOLEAN DisallowedDriveLetters[L'Z'-L'A'+1];





//
// Global variables
//
//extern ACTIVATE_INFO  g_ActivateInfo;


/*typedef struct _FILE_ZERO_DATA_INFORMATION {
  LARGE_INTEGER FileOffset;
  LARGE_INTEGER BeyondFinalZero;
} FILE_ZERO_DATA_INFORMATION, *PFILE_ZERO_DATA_INFORMATION;*/

extern PDRIVER_OBJECT   PushDriverObject;
extern PKEVENT          ProcessEvent;
extern PKEVENT          ThreadEvent;
extern PKEVENT          ImageEvent;
