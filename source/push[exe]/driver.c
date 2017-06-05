#include <sl.h>
#include <sldriver.h>
#include <slresource.h>
#include <slregistry.h>

#include "push.h"
#include "ring0.h"


//#define WRITE_DAC   0x00040000L

typedef VOID* PSECURITY_DESCRIPTOR;
extern UINT32 PushProcessId;
//#define SECURITY_DESCRIPTOR_REVISION     (1)
#define DACL_SECURITY_INFORMATION        (0x00000004L)
#define READ_CONTROL                     (0x00020000L)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))
#define REG_BINARY                  ( 3 )   // Free form binary

#define STATUS_INVALID_IMAGE_HASH       ((NTSTATUS)0xC0000428L)
#define STATUS_DRIVER_BLOCKED_CRITICAL  ((NTSTATUS)0xC000036BL)

#define MB_ICONQUESTION 0x00000020L
#define MB_YESNO        0x00000004L

#define IDYES 6


NTSTATUS __stdcall RtlCreateSecurityDescriptor(
    VOID* SecurityDescriptor,
    ULONG Revision
    );

NTSTATUS __stdcall RtlSetDaclSecurityDescriptor(
    VOID* SecurityDescriptor,
    BOOLEAN DaclPresent,
    ACL* Dacl,
    BOOLEAN DaclDefaulted
    );

NTSTATUS __stdcall RtlMakeSelfRelativeSD(
    VOID* AbsoluteSecurityDescriptor,
    VOID* SelfRelativeSecurityDescriptor,
    ULONG* BufferLength
    );

NTSTATUS __stdcall NtSetSecurityObject(
    HANDLE Handle,
    ULONG SecurityInformation,
    VOID* SecurityDescriptor
    );


VOID Driver_Extract()
{
    INTBOOL isWow64 = FALSE;

    NtQueryInformationProcess(
        NtCurrentProcess(),
        ProcessWow64Information,
        &isWow64,
        sizeof(INTBOOL),
        NULL
        );

    if (isWow64)
        Resource_Extract(L"DRIVER64", L"push0.sys");
    else
        Resource_Extract(L"DRIVER32", L"push0.sys");
}


VOID StripPermissions( WCHAR* KeyName )
{
    HANDLE keyHandle;
    PSECURITY_DESCRIPTOR psecdesc;
    VOID* selfSecurityDescriptor;
    ULONG bufferLength = 20;

    psecdesc = Memory_Allocate(9000);

    keyHandle = Registry_OpenKey(KeyName, WRITE_DAC);

    RtlCreateSecurityDescriptor(psecdesc, SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(psecdesc, TRUE, NULL, TRUE);

    selfSecurityDescriptor = Memory_Allocate(20);

    RtlMakeSelfRelativeSD(psecdesc, selfSecurityDescriptor, &bufferLength);
    NtSetSecurityObject(keyHandle, DACL_SECURITY_INFORMATION, selfSecurityDescriptor);
    NtClose(keyHandle);
}


VOID Driver_Load()
{
    NTSTATUS status;

    status = SlLoadDriver(
        L"PUSH",
        L"push0.sys",
        L"Push Kernel-Mode Driver",
        L"\\\\.\\Push",
        TRUE,
        &R0DriverHandle
        );

    if (!NT_SUCCESS(status))
    {
        Log(L"failed to load driver");

        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            Log(L"Driver file not found!");
        }

        if (status == STATUS_DRIVER_BLOCKED_CRITICAL)
        {
            // Probably wrong driver. Overwrite.

            File_Delete(L"push0.sys");
            Driver_Extract();

            // Try again.
            Driver_Load();
        }

        if (status == STATUS_INVALID_IMAGE_HASH)
        {
            INT32 msgId;

            // Prompt user to enable test-signing.
            msgId = MessageBoxW(
                NULL,
                L"The driver failed to load because it isn't signed. "
                L"It is required for Push to work correctly. "
                L"Do you want to enable test-signing to be able to use driver functions?",
                L"Driver Signing",
                MB_ICONQUESTION | MB_YESNO
                );

            if (msgId == IDYES)
            {
                HANDLE keyHandle;
                DWORD size = 255;
                WCHAR buffer[255];
                BYTE value = 0x01;
                UNICODE_STRING valueName;
                SYSTEM_BOOT_ENVIRONMENT_INFORMATION bootEnvironmentInformation;
                UINT32 returnLength;
                UNICODE_STRING guidAsUnicodeString;
                WCHAR guidAsWideChar[40];
                ULONG bufferLength = 20;
                OBJECT_ATTRIBUTES objectAttributes;
                UNICODE_STRING keyName;
                ULONG disposition;

                // Get boot GUID.

                NtQuerySystemInformation(
                    SystemBootEnvironmentInformation,
                    &bootEnvironmentInformation,
                    sizeof(SYSTEM_BOOT_ENVIRONMENT_INFORMATION),
                    &returnLength
                    );

                RtlStringFromGUID(
                    &bootEnvironmentInformation.BootIdentifier,
                    &guidAsUnicodeString
                    );

                String_CopyN(guidAsWideChar, guidAsUnicodeString.Buffer, 39);

                guidAsWideChar[39] = L'\0';

                String_Format(
                    buffer,
                    255,
                    L"\\Registry\\Machine\\BCD00000000\\Objects\\%s\\Elements",
                    guidAsWideChar
                    );

                // Change key permissions to allow us to create sub keys.
                StripPermissions(buffer);

                // Enable test-signing mode.

                String_Concatenate(buffer, L"\\16000049");

                // Change key permissions (if it already exists) to allow us to set values.
                StripPermissions(buffer);

                UnicodeString_Init(&keyName, buffer);
                UnicodeString_Init(&valueName, L"Element");

                objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
                objectAttributes.RootDirectory = NULL;
                objectAttributes.ObjectName = &keyName;
                objectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
                objectAttributes.SecurityDescriptor = NULL;
                objectAttributes.SecurityQualityOfService = NULL;

                // Create(NtCreateKey) the key not open(NtOpenKey) it because the key isn't
                // always there.
                NtCreateKey(
                    &keyHandle,
                    KEY_WRITE,
                    &objectAttributes,
                    0,
                    NULL,
                    0,
                    &disposition
                    );

                NtSetValueKey(keyHandle, &valueName, 0, REG_BINARY, &value, sizeof(BYTE));
                NtClose(keyHandle);

                MessageBoxW(
                    NULL,
                    L"Restart your computer to load driver",
                    L"Restart required",
                    NULL
                    );
            }
        }
    }
}


#define HKEY_LOCAL_MACHINE (( VOID* ) (UINT_B)((LONG)0x80000002) )
#define STANDARD_RIGHTS_ALL              (0x001F0000L)
#define KEY_QUERY_VALUE         (0x0001)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)
#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY         |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY                 |\
                                  KEY_CREATE_LINK)            \
                                  &                           \
                                 (~SYNCHRONIZE))
#define REG_SZ                      ( 1 )   // Unicode nul terminated string
#define REG_DWORD                   ( 4 )   // 32-bit number
#define INSTANCE_NAME L"ProtectorInstance"
#define INSTANCE_ALTITUDE L"265000"
#define INSTANCE_FLAGS  0
typedef DWORD REGSAM;
#define SE_PRIVILEGE_ENABLED            (0x00000002L)
#define SE_LOAD_DRIVER_PRIVILEGE            (10L)

LONG __stdcall RegCloseKey(
  VOID* hKey
);
LONG __stdcall RegSetValueW(
  VOID* hKey,
  WCHAR* lpSubKey,
  DWORD dwType,
  WCHAR* lpData,
  DWORD cbData
);
LONG __stdcall RegSetValueExW(
  VOID* hKey,
  WCHAR* lpValueName,
  DWORD Reserved,
  DWORD dwType,
  const BYTE *lpData,
  DWORD cbData
);

LONG __stdcall RegCreateKeyExW(
  VOID* hKey,
  WCHAR* lpSubKey,
  DWORD Reserved,
  WCHAR* lpClass,
  DWORD dwOptions,
  REGSAM samDesired,
    SECURITY_ATTRIBUTES* lpSecurityAttributes,
  VOID** phkResult,
  DWORD* lpdwDisposition
);
LONG __stdcall RegOpenKeyExW(
  VOID* hKey,
  WCHAR* lpSubKey,
  DWORD ulOptions,
  REGSAM samDesired,
  VOID** phkResult
);


BOOLEAN RegisterInstance( WCHAR* ServiceName )
{
    VOID *hKeyService=NULL, *hKeyInstances=NULL, *hKeyInstance=NULL;
    BOOLEAN result = FALSE;

    do
    {
        DWORD dwDisposition;
        DWORD dwFlags=INSTANCE_FLAGS;
        WCHAR buffer[260];

        String_Copy(buffer, L"System\\CurrentControlSet\\Services\\");
        String_Concatenate(buffer, ServiceName);

        if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            buffer,
            0,
            KEY_CREATE_SUB_KEY,&hKeyService)!=ERROR_SUCCESS)
            break;

        RegCreateKeyExW(
            hKeyService,
            L"Instances",
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyInstances,
            &dwDisposition
            );

        if(!hKeyInstances)
            break;

        if (RegSetValueExW(
            hKeyInstances,
            L"DefaultInstance",
            0,
            REG_SZ,
            (const BYTE*)INSTANCE_NAME,
            (String_GetLength(INSTANCE_NAME)+1)*sizeof(wchar_t))!=ERROR_SUCCESS)
            break;

        RegCreateKeyExW(
            hKeyInstances,
            INSTANCE_NAME,
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyInstance,
            &dwDisposition
            );

        if(!hKeyInstance)
            break;
        if(RegSetValueExW(
            hKeyInstance,
            L"Altitude",
            0,
            REG_SZ,
            (const BYTE*)INSTANCE_ALTITUDE,
            (String_GetLength(INSTANCE_ALTITUDE)+1)*sizeof(wchar_t))!=ERROR_SUCCESS)
            break;

        if(RegSetValueExW(
            hKeyInstance,
            L"Flags",
            0,
            REG_DWORD,
            (const BYTE*)&dwFlags,
            sizeof(DWORD))!=ERROR_SUCCESS)
            break;
        result=TRUE;
    }while(0);
    if(hKeyInstance)
        RegCloseKey(hKeyInstance);
    if(hKeyInstances)
        RegCloseKey(hKeyInstances);
    if(hKeyService)
        RegCloseKey(hKeyService);
    return result;
}


void StartServiceAvi (WCHAR *pszServiceName)
{
    void *hSCManager;
    void *hService;
    INTBOOL result;

    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    hService = OpenServiceW(hSCManager, pszServiceName, SERVICE_ALL_ACCESS);
    result = StartServiceW(hService, 0, NULL);

    CloseServiceHandle(hSCManager);
    CloseServiceHandle(hService);
}


NTSTATUS SlLoadDriver(
    WCHAR* ServiceName,
    WCHAR* DriverBinaryName,
    WCHAR* DisplayName,
    WCHAR* DeviceName,
    BOOLEAN Filter,
    HANDLE* DriverHandle
    )
{
    VOID *serviceHandle, *scmHandle, *fileHandle = NULL;
    WCHAR registyPath[260];
    DWORD dwSize;
    QUERY_SERVICE_CONFIG *lpServiceConfig;
    SERVICE_STATUS ServiceStatus;
    NTSTATUS status;
    WCHAR driverPath[260];
    WCHAR root[4];
    WCHAR dir[260];
    WCHAR *ptr;

    String_Copy(
        registyPath, 
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
        );

    if (!DriverBinaryName)
    {
        StartServiceAvi(L"PUSH");

        status = File_Create(
            &fileHandle,
            DeviceName,
            FILE_READ_ATTRIBUTES | GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
            NULL,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            *DriverHandle = fileHandle;

            return STATUS_SUCCESS;
        }
    }

    String_Copy(dir, PushFilePath);

    if((ptr = String_FindLastChar(dir, '\\')) != NULL)
    {
        *ptr = '\0';
    }

    String_Format(driverPath, 260, L"%s\\%s", dir, DriverBinaryName);

    root[0] = driverPath[0];
    root[1] = ':';
    root[2] = '\\';
    root[3] = '\0';

    if(root[0] == '\\' || GetDriveTypeW((WCHAR*)root) == DRIVE_REMOTE)
    {
        WCHAR tempPath[260];

        GetTempPathW(260, tempPath);
        String_Concatenate(tempPath, DriverBinaryName);
        File_Copy(driverPath, tempPath, NULL);
        String_Copy(driverPath, tempPath);
    }

    scmHandle = OpenSCManagerW(0, 0, SC_MANAGER_ALL_ACCESS);

    serviceHandle = OpenServiceW(
        scmHandle,
        ServiceName,
        SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS | SERVICE_START
        );

    if (serviceHandle == NULL)
    {
        serviceHandle = CreateServiceW(
            scmHandle,
            ServiceName,
            DisplayName,
            SERVICE_ALL_ACCESS,
            SERVICE_KERNEL_DRIVER,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            driverPath,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
            );
    }

    if (serviceHandle == NULL)
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (DriverBinaryName)
    {
        QueryServiceConfigW(serviceHandle, 0, 0, &dwSize);

        lpServiceConfig = (QUERY_SERVICE_CONFIG *)Memory_AllocateEx(
            dwSize,
            HEAP_ZERO_MEMORY
            );

        QueryServiceConfigW(serviceHandle, lpServiceConfig, dwSize, &dwSize);

        if (String_Compare(driverPath, lpServiceConfig->lpBinaryPathName + 4) != 0)
        {
            ChangeServiceConfigW(
                serviceHandle,
                SERVICE_NO_CHANGE,
                SERVICE_NO_CHANGE,
                SERVICE_NO_CHANGE,
                driverPath,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
                );
        }

        Memory_Free(lpServiceConfig);
    }

    QueryServiceStatus(serviceHandle, &ServiceStatus);

    if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
    {
        UNICODE_STRING u_str;
        VOID *tokenHandle;
        VOID *processHandle = 0;
        TOKEN_PRIVILEGES NewState;
        OBJECT_ATTRIBUTES objectAttributes = {0};
        CLIENT_ID clientId = {0};
        NTSTATUS status;

        objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);

        clientId.UniqueProcess = PushProcessId;

        NtOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            &objectAttributes,
            &clientId
            );

        NtOpenProcessToken(processHandle,
            0x20 | 0x0008, // TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
            &tokenHandle);

        NewState.PrivilegeCount = 1;

        NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        NewState.Privileges[0].Luid.HighPart = 0;
        NewState.Privileges[0].Luid.LowPart = SE_LOAD_DRIVER_PRIVILEGE;

        NtAdjustPrivilegesToken(tokenHandle,
            FALSE,
            &NewState,
            sizeof(TOKEN_PRIVILEGES),
            NULL,
            NULL);

        NtClose(tokenHandle);

        String_Concatenate(registyPath, ServiceName);

        UnicodeString_Init(&u_str, registyPath);

        if (Filter)
        {
            RegisterInstance(ServiceName);
        }

       status = NtLoadDriver(&u_str);
       StartServiceW(serviceHandle, 0, NULL);

       if (!NT_SUCCESS(status))
       {
           return status;
       }
    }

    CloseServiceHandle(scmHandle);
    CloseServiceHandle(serviceHandle);

    status = File_Create(
        &fileHandle,
        DeviceName,
        FILE_READ_ATTRIBUTES | GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        NULL,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    *DriverHandle = fileHandle;

    return STATUS_SUCCESS;
}
