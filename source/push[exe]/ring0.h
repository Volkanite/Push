DWORD GetGpuVendorID(
    );

VOID LoadDriver(
    );

VOID PushSetCacheName(
    CHAR* Game
    );

#ifdef __cplusplus
extern "C" {
#endif

VOID PushToggleProcessMonitoring(
    BOOLEAN Activate
    );

VOID PushGetProcessInfo(
    PROCESS_CALLBACK_INFO* ProcessInformation
    );

VOID PushGetThreadInfo(
    THREAD_CALLBACK_INFO* ThreadInformation
    );

VOID PushGetImageInfo(
    IMAGE_CALLBACK_INFO* ImageInformation
    );

DWORD PushReadMsr(
    DWORD Index
    );

VOID PushReadPciConfig(
    DWORD   xPciAddress,
    DWORD   xRegAddress,
    BYTE    *value,
    UINT32  size
    );

DWORD PushReadPhysicalMemory(
    DWORD Address
    );

VOID* R0MapPhysicalMemory(
    DWORD phyAddr,
    DWORD memSize
    );

NTSTATUS R0CreateRamDisk(
    RAMDISK_CREATE_DATA* CreateData
    );

VOID R0QueryRamDisk(
    VOID* DeviceHandle,
    RAMDISK_CREATE_DATA* CreateData
    );

VOID
R0QueueFile(
    WCHAR* FileName,
    UINT32 FileNameLength
    );

#ifdef __cplusplus
}
#endif


// Bus Number, Device Number and Function Number to PCI Device Address
#define PciBusDevFunc(Bus, Dev, Func)   ((Bus&0xFF)<<8) | ((Dev&0x1F)<<3) | (Func&7)

#ifdef __cplusplus
extern "C" VOID *R0DriverHandle;
#else
extern VOID *R0DriverHandle;
#endif // __cplusplus

