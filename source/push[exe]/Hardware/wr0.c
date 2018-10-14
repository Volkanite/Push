#include <push.h>
#include "wr0.h"


//Driver Show Name
#define DRIVER_ID       L"WinRing0"
#define OLS_DRIVER_ID   L"WinRing0_1_2_0"


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

    /*gHandle = CreateFile(
        _T("\\\\.\\") OLS_DRIVER_ID,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );*/

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
// PCI Configuration Read
//
//-----------------------------------------------------------------------------

BOOL Wr0ReadPciConfig( DWORD pciAddress, DWORD regAddress, BYTE* value, DWORD size )
{
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

    DWORD   returnedLength = 0;
    OLS_READ_PCI_CONFIG_INPUT inBuf;
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    inBuf.PciAddress = pciAddress;
    inBuf.PciOffset = regAddress;

    /*result = DeviceIoControl(
        gHandle,
        IOCTL_OLS_READ_PCI_CONFIG,
        &inBuf,
        sizeof(inBuf),
        value,
        size,
        &returnedLength,
        NULL
        );*/

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