#include <sltypes.h>
#include <slnt.h>
#include <slntuapi.h>
#include <pushbase.h>

#include "ring0.h"


DWORD
PushReadPhysicalMemory( DWORD Address )
{
    DWORD   buffer;
    //UINT32  iBytesRet;
    IO_STATUS_BLOCK isb;

    /*DeviceIoControl(PushDriverHandle,
                    IOCTL_PUSH_READ_GPU_REGISTER,
                    &xAddr, sizeof(DWORD),
                    &dwBuffer, sizeof(DWORD),
                    &iBytesRet,
                    NULL);*/

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
}


//map physical memory to user space
VOID*
R0MapPhysicalMemory(
    DWORD phyAddr,
    DWORD memSize
    )
{
    UINT64 virtualAddress=NULL;   //mapped virtual addr
    //DWORD dwBytes=0;
    //BOOLEAN bRet=FALSE;
    PHYSICAL_MEMORY physicalMemory;
    IO_STATUS_BLOCK isb;

    physicalMemory.pvAddr= phyAddr; //physical address
    physicalMemory.dwSize=memSize;  //memory size

    /*bRet=DeviceIoControl(
            PushDriverHandle,
            IOCTL_PUSH_MAP_PHYSICAL_MEMORY,
            &pm,
            sizeof(PHYMEM_MEM),
            &pVirAddr,
            sizeof(UINT64),
            &dwBytes,
            NULL
            );*/

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

    /*if (dwBytes==sizeof(UINT64))
        return virtualAddress;
    else
        return NULL;*/

    return virtualAddress;
}


VOID
PushReadPciConfig(
    DWORD   xPciAddress,
    DWORD   xRegAddress,
    BYTE    *value,
    UINT32  size
    )
{
    UINT32 returnedLength = 0;
    READ_PCI_CONFIG_INPUT   inputBuffer;
    IO_STATUS_BLOCK isb;


    inputBuffer.PciAddress = xPciAddress;
    inputBuffer.PciOffset = xRegAddress;

    /*DeviceIoControl(PushDriverHandle,
                    IOCTL_PUSH_READ_PCI_CONFIG,
                    &inBuf, sizeof(inBuf),
                    value, size,
                    &returnedLength,
                    NULL
                    );*/

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_READ_PCI_CONFIG,
        &inputBuffer,
        sizeof(inputBuffer),
        value,
        size
        );
}


DWORD
PushReadMsr( DWORD Index )
{
    IO_STATUS_BLOCK isb;
    QWORD eax;

    NtDeviceIoControlFile(
        R0DriverHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_PUSH_READ_MSR,
        &Index,
        sizeof(DWORD),
        &eax,
        sizeof(eax)
        );

    return eax;
}


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

