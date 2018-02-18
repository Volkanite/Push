#pragma once

///
/// The base names for the device objects created in \Device
///
#define PUSH_DEVICE_DIR_NAME                        L"\\Device"
#define PUSH_DEVICE_BASE_NAME                       PUSH_DEVICE_DIR_NAME L"\\Push"

///
/// The symlinks to the device objects created in \DosDevices
///
#define PUSH_DOSDEV                                 L"Push"
#define PUSH_DOSDEV_NAME                            L"\\\\.\\" PUSH_DOSDEV
#define PUSH_SYMLINK_NAME                           L"\\DosDevices\\" PUSH_DOSDEV

///
/// Shared memory section name
///
#define PUSH_SECTION_NAME                           L"PushSharedMemory"

///
/// Events
///
#define PUSH_PROCESS_EVENT_NAME                     L"PushProcessEvent"
#define PUSH_THREAD_EVENT_NAME                      L"PushThreadEvent"
#define PUSH_IMAGE_EVENT_NAME                       L"PushImageEvent"
#define PUSH_RAMDISK_REMOVE_EVENT_NAME              L"PushRemoveRamDiskEvent"

///
/// RAM Disk
///
#define PUSH_RAMDISK_DEVICE_NAME                    L"\\Device\\PushRamDisk"

///
/// Settings File
///
#define PUSH_SETTINGS_FILE                          L"push.ini"

///
/// Push version information
///
#define PUSH_VERSION                                L"r43"

///
/// Base value for the IOCTL's.
///
#define FILE_DEVICE_PUSH             0x8000
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define IOCTL_PUSH_READ_MSR             CTL_CODE(FILE_DEVICE_PUSH, 0x800, 0, 0)
#define IOCTL_PUSH_READ_PCI_CONFIG      CTL_CODE(FILE_DEVICE_PUSH, 0x801, 0, ( 0x0001 ))
#define IOCTL_PUSH_READ_GPU_REGISTER    CTL_CODE(FILE_DEVICE_PUSH, 0x802, 0, 0)
#define IOCTL_PUSH_SET_CACHE_NAME       CTL_CODE(FILE_DEVICE_PUSH, 0x803, 0, 0)
#define IOCTL_PUSH_QUERY_VERSION        CTL_CODE(FILE_DEVICE_PUSH, 0x804, 0, 0)
#define IOCTL_PUSH_CREATE_RAMDISK       CTL_CODE(FILE_DEVICE_PUSH, 0x805, 0, ( 0x0001 ) | ( 0x0002 ))
#define IOCTL_PUSH_QUERY_RAMDISK        CTL_CODE(FILE_DEVICE_PUSH, 0x806, 0, 0)
#define IOCTL_PUSH_QUERY_DRIVER         CTL_CODE(FILE_DEVICE_PUSH, 0x807, 0, 0)
#define IOCTL_PUSH_TOGGLE_MONITORING    CTL_CODE(FILE_DEVICE_PUSH, 0x808, 0, ( 0x0001 ) | ( 0x0002 ))
#define IOCTL_PUSH_GET_PROC_INFO        CTL_CODE(FILE_DEVICE_PUSH, 0x809, 0, ( 0x0001 ) | ( 0x0002 ))
#define IOCTL_PUSH_GET_IMAGE_INFO       CTL_CODE(FILE_DEVICE_PUSH, 0x810, 0, ( 0x0001 ) | ( 0x0002 ))
#define IOCTL_PUSH_GET_THREAD_INFO      CTL_CODE(FILE_DEVICE_PUSH, 0x811, 0, ( 0x0001 ) | ( 0x0002 ))
#define IOCTL_PUSH_MAP_PHYSICAL_MEMORY  CTL_CODE(FILE_DEVICE_PUSH, 0x812, 0, ( 0x0001 ) | ( 0x0002 ))
#define IOCTL_PUSH_QUEUE_FILE           CTL_CODE(FILE_DEVICE_PUSH, 0x813, 0, ( 0x0001 ) | ( 0x0002 ))


#define IMDISK_VERSION                 0x0160
#define IMDISK_DRIVER_VERSION          0x0103

#ifndef ZERO_STRUCT
#define ZERO_STRUCT { 0 }
#endif



///
/// Bit constants for the Flags field in IMDISK_CREATE_DATA
///

/// Read-only device
#define IMDISK_OPTION_RO                0x00000001

/// Check if flags specifies read-only
#define IMDISK_READONLY(x)              ((ULONG)(x) & 0x00000001)

/// Removable, hot-plug, device
#define IMDISK_OPTION_REMOVABLE         0x00000002

/// Check if flags specifies removable
#define IMDISK_REMOVABLE(x)             ((ULONG)(x) & 0x00000002)

/// Specifies that image files are created with sparse attribute.
#define IMDISK_OPTION_SPARSE_FILE       0x00000004

/// Check if flags specifies sparse
#define IMDISK_SPARSE_FILE(x)           ((ULONG)(x) & 0x00000004)

/// Device type is virtual harddisk partition
#define IMDISK_DEVICE_TYPE_HD           0x00000010
/// Device type is virtual floppy drive
#define IMDISK_DEVICE_TYPE_FD           0x00000020
/// Device type is virtual CD/DVD-ROM drive
#define IMDISK_DEVICE_TYPE_CD           0x00000030

/// Extracts the IMDISK_DEVICE_TYPE_xxx from flags
#define IMDISK_DEVICE_TYPE(x)           ((ULONG)(x) & 0x000000F0)

/// Virtual disk is backed by image file
#define IMDISK_TYPE_FILE                0x00000100
/// Virtual disk is backed by virtual memory
#define IMDISK_TYPE_VM                  0x00000200
/// Virtual disk is backed by proxy connection
#define IMDISK_TYPE_PROXY               0x00000300

/// Extracts the IMDISK_TYPE_xxx from flags
#define IMDISK_TYPE(x)                  ((ULONG)(x) & 0x00000F00)

/// Proxy connection is direct-type
#define IMDISK_PROXY_TYPE_DIRECT        0x00000000
/// Proxy connection is over serial line
#define IMDISK_PROXY_TYPE_COMM          0x00001000
/// Proxy connection is over TCP/IP
#define IMDISK_PROXY_TYPE_TCP           0x00002000
/// Proxy connection uses shared memory
#define IMDISK_PROXY_TYPE_SHM           0x00003000

/// Extracts the IMDISK_PROXY_TYPE_xxx from flags
#define IMDISK_PROXY_TYPE(x)            ((ULONG)(x) & 0x0000F000)

/// Extracts the IMDISK_PROXY_TYPE_xxx from flags
#define IMDISK_IMAGE_MODIFIED           0x00010000




typedef struct _IMDISK_SET_DEVICE_FLAGS
{
  unsigned long FlagsToChange;
  unsigned long FlagValues;
} IMDISK_SET_DEVICE_FLAGS, *PIMDISK_SET_DEVICE_FLAGS;

#define IMDISK_API_NO_BROADCAST_NOTIFY  0x00000001
#define IMDISK_API_FORCE_DISMOUNT       0x00000002

//OSD Flags
#define OSD_TIME            0x00000001
#define OSD_GPU_LOAD        0x00000002
#define OSD_GPU_TEMP        0x00000004
#define OSD_GPU_VRAM        0x00000008
#define OSD_CPU_LOAD        0x00000010
#define OSD_CPU_TEMP        0x00000020
#define OSD_RAM             0x00000040
#define OSD_MCU             0x00000080
#define OSD_MTU             0x00000100
#define OSD_CPU_SPEED       0x00000200
#define OSD_FPS             0x00000400
#define OSD_DISK_RWRATE     0x00000800
#define OSD_DISK_RESPONSE   0x00001000
#define OSD_GPU_E_CLK       0x00002000
#define OSD_GPU_M_CLK       0x00004000
#define OSD_GPU_FAN_RPM     0x00008000
#define OSD_BUFFERS         0x00010000
#define OSD_RESOLUTION      0x00020000
#define OSD_GPU_VOLTAGE     0x00040000
#define OSD_GPU_FAN_DC      0x00080000
#define OSD_REFRESH_RATE    0x00100000
#define OSD_LAST_ITEM       0x00100000 //Update me!


typedef struct _CORE_LIST CORE_LIST;
typedef struct _CORE_LIST {

    UINT8 number;
    LARGE_INTEGER perfCounter;
    LARGE_INTEGER idleTime;
    float usage;
    CORE_LIST *nextEntry;

} CORE_LIST;


#pragma pack(push,1)
typedef struct _PUSH_HARDWARE_INFORMATION
{
    struct
    {
        UINT8   Temperature;
        UINT8   Load;
        UINT32  EngineClock;
        UINT32  MemoryClock;
        UINT32  EngineClockMax;
        UINT32  MemoryClockMax;
        UINT32  EngineOverclock;
        UINT32  MemoryOverclock;
        UINT32  Voltage;
        UINT32  VoltageMax;
        UINT32  FanSpeed;
        UINT8   FanDutyCycle;

        ULONG   pciAddress;
        ULONG   BarAddress;

        UINT16  VendorId;

        struct
        {
            UINT8 Load;
            UINT32 Used;
            UINT32 Total;

        }FrameBuffer;

    }DisplayDevice;

    struct
    {
        UINT8 RefreshRate;

    }Display;

    struct
    {
        UINT8 NumberOfCores;
        UINT16 MhzCurrent;
        UINT16 MhzMax;
        UINT8 Load;
        UINT8 MaxThreadUsage;
        UINT8 MaxCoreUsage;
        UINT8 Temperature;
        INT32 tjmax;
    }Processor;

    struct
    {
        UINT8 Load;
        UINT32 Used;
        UINT32 Total;

    }Memory;

    struct
    {
        UINT32 ReadWriteRate;
        UINT32 ResponseTime;
    }Disk;

}PUSH_HARDWARE_INFORMATION;
#pragma pack(pop)


typedef enum _PUSH_VSYNC_OVERRIDE_MODE
{
    PUSH_VSYNC_UNCHANGED = 0,
    PUSH_VSYNC_FORCE_ON,
    PUSH_VSYNC_FORCE_OFF,

} PUSH_VSYNC_OVERRIDE_MODE;

typedef enum _KEYBOARD_HOOK_TYPE
{
    KEYBOARD_HOOK_AUTO,
    KEYBOARD_HOOK_SUBCLASS,
    KEYBOARD_HOOK_MESSAGE,
    KEYBOARD_HOOK_KEYBOARD,
    KEYBOARD_HOOK_DETOURS,
    KEYBOARD_HOOK_RAW

} PUSH_KEYBOARD_HOOK_TYPE;


typedef VOID(*OSD_DYNAMIC_FORMAT)(
    UINT32 Value,
    WCHAR* Buffer
    );

typedef UINT16 OSDVALUE;


#pragma pack(push,1)
typedef struct _OSD_ITEM
{
    UINT32 Flag;
    OSDVALUE Threshold;
    UINT8 ValueSize;
    UINT32 Value;
    UINT32 Value2;
    UINT32 Color;
    WCHAR Text[20];
    WCHAR Description[40];

    //LOCAL //fix //pointers can only reside locally (push.exe)?
    ULONG DisplayFormatPtr; // WCHAR* DisplayFormat;
    ULONG DynamicFormatPtr; // OSD_DYNAMIC_FORMAT DynamicFormat; //For when formatting must happen at runtime
    ULONG ValueSourcePtr;   // VOID* ValueSource;
    ULONG ValueSource2Ptr;  // UINT32* ValueSource2;

}OSD_ITEM;


// Structure for shared memory

typedef struct _PUSH_SHARED_MEMORY
{
    //OSD
    UINT32  OSDFlags;
    UINT32  Overloads;
    UINT8   NumberOfOsdItems;

    UCHAR                       FrameLimit;
    PUSH_VSYNC_OVERRIDE_MODE    VsyncOverrideMode;
    PUSH_KEYBOARD_HOOK_TYPE     KeyboardHookType;
    UCHAR                       DisableRepeatKeys;
    UCHAR                       SwapWASD;
    UCHAR                       KeepFps;
    UCHAR                       GameUsesRamDisk;
    long long                   performanceFrequency;
    BOOLEAN                     ThreadOptimization;
    BOOLEAN                     LogFileIo;
    BOOLEAN                     AutoLogFileIo;
    UCHAR                       ControllerTimeout;
    PUSH_HARDWARE_INFORMATION   HarwareInformation;
    WCHAR                       FontName[60];
    BOOLEAN                     FontBold;
    OSD_ITEM                    OsdItems[1];

} PUSH_SHARED_MEMORY;
#pragma pack(pop)


#ifndef PROCESSID
#define PROCESSID unsigned long
#endif
typedef struct _PROCESS_CALLBACK_INFO
{
    PROCESSID hProcessID;

} PROCESS_CALLBACK_INFO, *PPROCESS_CALLBACK_INFO;


typedef struct _THREAD_CALLBACK_INFO
{
    UINT32 threadOwner;
    UINT16 threadID;
    unsigned char create;

}  THREAD_CALLBACK_INFO;


typedef struct _IMAGE_CALLBACK_INFO
{
    PROCESSID processID;

} IMAGE_CALLBACK_INFO;


/**
   Structure used by the IOCTL_IMDISK_CREATE_DEVICE and
   IOCTL_IMDISK_QUERY_DEVICE calls and by the ImDiskQueryDevice() function.
*/

typedef struct _RAMDISK_CREATE_DATA
{
  /// Total size in bytes (in the Cylinders field) and virtual geometry.
  DISK_GEOMETRY   DiskGeometry;
  /// The byte offset in the image file where the virtual disk begins.
  //LARGE_INTEGER   ImageOffset;
  /// Creation flags. Type of device and type of connection.
  //unsigned long           Flags;
  /// Driveletter (if used, otherwise zero).
  unsigned short           DriveLetter;
  /// Length in bytes of the FileName member.
  //unsigned short          FileNameLength;
  /// Dynamically-sized member that specifies the image file name.
  //unsigned short           FileName[1];
} RAMDISK_CREATE_DATA;


typedef struct _READ_PCI_CONFIG_INPUT
{
    unsigned long PciAddress;
    unsigned long PciOffset;

}   READ_PCI_CONFIG_INPUT;


typedef struct _PHYSICAL_MEMORY
{
    UINT64 pvAddr;   //physical addr when mapping, virtual addr when unmapping
    ULONG dwSize;   //memory size to map or unmap
} PHYSICAL_MEMORY;
