#include <push.h>
#include "wr0.h"


//Driver Show Name
#define DRIVER_ID       L"WinRing0"
#define OLS_DRIVER_ID   L"WinRing0_1_0_1"


BOOL Wr0InstallDriver(WCHAR* DriverId, WCHAR* DriverPath);
BOOL RemoveDriver(WCHAR* DriverId);
BOOL StartDriver(WCHAR* DriverId);
BOOL StopDriver(WCHAR* DriverId);
BOOL OpenDriver();


#define SC_HANDLE HANDLE
#define ERROR_SERVICE_EXISTS             1073L
#define ERROR_SERVICE_ALREADY_RUNNING    1056L
#define SERVICE_CONTROL_STOP                   0x00000001
BOOL __stdcall DeleteService(
    SC_HANDLE hService
    );


HANDLE gHandle = INVALID_HANDLE_VALUE;
BOOLEAN Wr0DriverLoaded;


//-----------------------------------------------------------------------------
//
// Initialize
//
//-----------------------------------------------------------------------------

//please use absolute file path
BOOL Wr0Initialize( WCHAR* DriverPath )
{
    //32 bit system use 32 bit dirver
    //64 bit system use 64 bit dirver
    //BOOL bIsWow64 = FALSE;
    //IsWow64Process(GetCurrentProcess(), &bIsWow64);
    //if (bIsWow64)InstallDriver(DRIVER_ID, DRIVER_x64);
    //else InstallDriver(DRIVER_ID, DRIVER_x86);
    if (Wr0InstallDriver(DRIVER_ID, DriverPath))
    if (StartDriver(DRIVER_ID))
    if (OpenDriver())return TRUE;
    return FALSE;
}


//-----------------------------------------------------------------------------
//
// Deinitialize
//
//-----------------------------------------------------------------------------

BOOL Deinitialize()
{
    StopDriver(DRIVER_ID);
    RemoveDriver(DRIVER_ID);
    return TRUE;
}


//-----------------------------------------------------------------------------
//
// Install Driver
//
//-----------------------------------------------------------------------------

BOOL Wr0InstallDriver(WCHAR* DriverId, WCHAR* DriverPath)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE   hService = NULL;
    BOOL        rCode = FALSE;
    DWORD       error = 0;

    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        return FALSE;
    }

    hService = CreateServiceW(hSCManager,
        DriverId,
        DriverId,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        DriverPath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (hService == NULL)
    {
        error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS)
        {
            rCode = TRUE;
        }
    }
    else
    {
        rCode = TRUE;
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCManager);
    return rCode;
}


//-----------------------------------------------------------------------------
//
// Remove Driver
//
//-----------------------------------------------------------------------------

BOOL RemoveDriver(WCHAR* DriverId)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE   hService = NULL;
    BOOL        rCode = FALSE;

    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        return FALSE;
    }

    hService = OpenServiceW(hSCManager, DriverId, SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
        rCode = TRUE;
    }
    else
    {
        rCode = DeleteService(hService);
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCManager);
    return rCode;
}


//-----------------------------------------------------------------------------
//
// Open Driver
//
//-----------------------------------------------------------------------------

BOOL OpenDriver()
{
    NTSTATUS status;

    status = File_Create(
        &gHandle,
        L"\\\\.\\" OLS_DRIVER_ID,
        FILE_READ_ATTRIBUTES | GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        NULL,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (gHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
//
// Start Driver
//
//-----------------------------------------------------------------------------

BOOL StartDriver(WCHAR* DriverId)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE   hService = NULL;
    BOOL        rCode = FALSE;
    DWORD       error = 0;

    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        return FALSE;
    }

    hService = OpenServiceW(hSCManager, DriverId, SERVICE_ALL_ACCESS);

    if (hService != NULL)
    {
        if (!StartServiceW(hService, 0, NULL))
        {
            if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
            {
                rCode = TRUE;
            }
        }
        else
        {
            rCode = TRUE;
        }
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCManager);
    return rCode;
}


//-----------------------------------------------------------------------------
//
// Stop Driver
//
//-----------------------------------------------------------------------------

BOOL StopDriver(WCHAR* DriverId)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE       hService = NULL;
    BOOL            rCode = FALSE;
    SERVICE_STATUS  serviceStatus;
    DWORD       error = 0;

    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        return FALSE;
    }

    hService = OpenServiceW(hSCManager, DriverId, SERVICE_ALL_ACCESS);

    if (hService != NULL)
    {
        rCode = ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCManager);
    return rCode;
}


//-----------------------------------------------------------------------------
//
// CPU
//
//-----------------------------------------------------------------------------

BOOL Wr0Rdmsr( DWORD Index, DWORD* EAX, DWORD* EDX )
{
    IO_STATUS_BLOCK isb;
    NTSTATUS status;
    BOOL    result = FALSE;
    BYTE buffer[8];

    if (gHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if (EAX == NULL || EDX == NULL)
    {
        return FALSE;
    }

    status = NtDeviceIoControlFile(
        gHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_OLS_READ_MSR,
        &Index,
        sizeof(DWORD),
        &buffer,
        sizeof(buffer)
        );

    if (NT_SUCCESS(status))
    {
        Memory_Copy(EAX, buffer, 4);
        Memory_Copy(EDX, buffer + 4, 4);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//-----------------------------------------------------------------------------
//
// PCI Configuration Read
//
//-----------------------------------------------------------------------------

BOOL Wr0ReadPciConfig( DWORD pciAddress, DWORD regAddress, BYTE* value, DWORD size )
{
    DWORD   returnedLength = 0;
    OLS_READ_PCI_CONFIG_INPUT inBuf;
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    if (gHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    if (value == NULL)
    {
        return FALSE;
    }
    // alignment check
    if (size == 2 && (regAddress & 1) != 0)
    {
        return FALSE;
    }
    if (size == 4 && (regAddress & 3) != 0)
    {
        return FALSE;
    }

    inBuf.PciAddress = pciAddress;
    inBuf.PciOffset = regAddress;

    status = NtDeviceIoControlFile(
        gHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_OLS_READ_PCI_CONFIG,
        &inBuf,
        sizeof(inBuf),
        value,
        size
        );

    if (NT_SUCCESS(status))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//-----------------------------------------------------------------------------
//
// Physical Memory
//
//-----------------------------------------------------------------------------

DWORD Wr0ReadPhysicalMemory( ULONG_PTR address, BYTE* buffer, DWORD count, DWORD unitSize )
{
    DWORD   size = 0;
    OLS_READ_MEMORY_INPUT inBuf;
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    if (gHandle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    if (buffer == NULL)
    {
        return 0;
    }

    if (sizeof(ULONG_PTR) == 4)
    {
        inBuf.Address.u.HighPart = 0;
        inBuf.Address.u.LowPart = (DWORD)address;
    }
    else
    {
        inBuf.Address.QuadPart = address;
    }

    inBuf.UnitSize = unitSize;
    inBuf.Count = count;
    size = inBuf.UnitSize * inBuf.Count;

    status = NtDeviceIoControlFile(
        gHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_OLS_READ_MEMORY,
        &inBuf,
        sizeof(OLS_READ_MEMORY_INPUT),
        buffer,
        size
        );

    if (NT_SUCCESS(status) && isb.Information == size)
    {
        return count * unitSize;
    }
    else
    {
        return 0;
    }
}
