#include <sl.h>

#include "push.h"
#include "ring0.h"


/*DWORD
PushReadPhysicalMemory( DWORD Address )
{
    DWORD   buffer;
    //UINT32  iBytesRet;
    IO_STATUS_BLOCK isb;

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_READ_GPU_REGISTER,
        &Address,
        sizeof(DWORD),
        &buffer,
        sizeof(DWORD)
        );

    return buffer;
}*/


//map physical memory to user space
VOID* R0MapPhysicalMemory( 
    DWORD phyAddr, 
    DWORD memSize 
    )
{
    QWORD virtualAddress=NULL;   
    PHYSICAL_MEMORY physicalMemory;
    IO_STATUS_BLOCK isb;

    physicalMemory.pvAddr= phyAddr; 
    physicalMemory.dwSize=memSize;

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_MAP_PHYSICAL_MEMORY,
        &physicalMemory,
        sizeof(PHYSICAL_MEMORY),
        &virtualAddress,
        sizeof(UINT64)
        );

    return (VOID*) virtualAddress;
}


/*VOID CPU_ReadMsr(DWORD Index, DWORD* EAX, DWORD* EDX)
{
    IO_STATUS_BLOCK isb;
    NTSTATUS status;
    BYTE buffer[8];

    status = NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_READ_MSR,
        &Index,
        sizeof(DWORD),
        &buffer,
        sizeof(buffer)
        );

    if (NT_SUCCESS(status))
    {
        Memory_Copy(EAX, buffer, 4);
        Memory_Copy(EDX, buffer + 4, 4);
    }
}*/


/*BOOLEAN
R0ReadPciConfig( 
    DWORD PciAddress, 
    DWORD RegAddress,
    BYTE* Value,
    UINT32 Size
    )
{
    UINT32 returnedLength = 0;
    READ_PCI_CONFIG_INPUT inputBuffer;
    IO_STATUS_BLOCK isb;
    NTSTATUS status;

    inputBuffer.PciAddress = PciAddress;
    inputBuffer.PciOffset = RegAddress;

    status = NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_READ_PCI_CONFIG,
        &inputBuffer,
        sizeof(inputBuffer),
        Value,
        Size
        );

    if (!NT_SUCCESS(status))
        return FALSE;
    else
        return TRUE;
}*/


VOID
PushToggleProcessMonitoring(
    BOOLEAN Activate
    )
{
    UINT32 iBytesReturned = 0;
    IO_STATUS_BLOCK isb;

    /*DeviceIoControl(PushDriverHandle,
                    IOCTL_PUSH_TOGGLE_MONITORING,
                    &bActivate, sizeof(BOOLEAN),
                    0,0,
                    &iBytesReturned,
                    0);*/

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_TOGGLE_MONITORING,
        &Activate,
        sizeof(BOOLEAN),
        NULL,
        NULL
        );
}


VOID
PushSetCacheName( CHAR* Game )
{
    DWORD buffer;
    IO_STATUS_BLOCK isb;

    /*DeviceIoControl(PushDriverHandle,
        IOCTL_PUSH_SET_CACHE_NAME,
        pszGame, sizeof(pszGame),
        &dwBuffer, sizeof(unsigned long),
        &dwBytesRet,
        NULL);*/

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_SET_CACHE_NAME,
        Game,
        sizeof(Game),
        &buffer,
        sizeof(buffer)
        );
}


VOID
PushGetProcessInfo(
    PROCESS_CALLBACK_INFO* ProcessInformation
    )
{
    IO_STATUS_BLOCK isb;

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_GET_PROC_INFO,
        NULL,
        NULL,
        ProcessInformation,
        sizeof(PROCESS_CALLBACK_INFO)
        );
}


VOID
PushGetThreadInfo(
    THREAD_CALLBACK_INFO* ThreadInformation
    )
{
    IO_STATUS_BLOCK isb;

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_GET_THREAD_INFO,
        NULL,
        NULL,
        ThreadInformation,
        sizeof(THREAD_CALLBACK_INFO)
        );
}


VOID
PushGetImageInfo(
    IMAGE_CALLBACK_INFO* ImageInformation
    )
{
    IO_STATUS_BLOCK isb;

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_GET_IMAGE_INFO,
        NULL,
        NULL,
        ImageInformation,
        sizeof(IMAGE_CALLBACK_INFO)
        );
}


NTSTATUS
R0CreateRamDisk(
    RAMDISK_CREATE_DATA* CreateData
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtDeviceIoControlFile(
                R0DriverHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                IOCTL_PUSH_CREATE_RAMDISK,
                CreateData,
                sizeof(RAMDISK_CREATE_DATA),
                CreateData,
                sizeof(RAMDISK_CREATE_DATA)
                );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(
                    R0DriverHandle,
                    FALSE,
                    0
                    );

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}


VOID
R0QueryRamDisk(
    VOID* DeviceHandle,
    RAMDISK_CREATE_DATA* CreateData
    )
{
    IO_STATUS_BLOCK isb;

    NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_QUERY_RAMDISK,
        NULL,
        NULL,
        CreateData,
        sizeof(RAMDISK_CREATE_DATA)
        );
}


VOID
R0QueueFile(
    WCHAR* FileName,
    UINT32 FileNameLength
    )
{
    IO_STATUS_BLOCK isb;

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_QUEUE_FILE,
        FileName,
        FileNameLength * sizeof(WCHAR),
        NULL,
        NULL
        );
}

