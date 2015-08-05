#include <sl.h>
#include <sldriver.h>
#include <slresource.h>
#include <slregistry.h>
#include <stdio.h>
#include <string.h>

#include "push.h"
#include "ring0.h"


#define WRITE_DAC   0x00040000L

typedef VOID* PSECURITY_DESCRIPTOR;

#define SECURITY_DESCRIPTOR_REVISION     (1)
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

extern "C"
{
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
}


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
        Resource::Extract(L"DRIVER64", L"push0.sys");
    else
        Resource::Extract(L"DRIVER32", L"push0.sys");
}


VOID StripPermissions( WCHAR* KeyName )
{
    HANDLE keyHandle;
    unsigned char p[9000];
    PSECURITY_DESCRIPTOR psecdesc = (PSECURITY_DESCRIPTOR)p;
    VOID* selfSecurityDescriptor;
    ULONG bufferLength = 20;

    keyHandle = Registry::OpenKey(KeyName, WRITE_DAC);

    RtlCreateSecurityDescriptor(psecdesc, SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(psecdesc, TRUE, NULL, TRUE);

    selfSecurityDescriptor = RtlAllocateHeap(PushHeapHandle, 0, 20);

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
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            MessageBoxW(NULL, L"Driver file not found!", L"Error", 0);
        }

        if (status == STATUS_DRIVER_BLOCKED_CRITICAL)
        {
            // Probably wrong driver. Overwrite.

            DeleteFileW(L"push0.sys");
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

                String::CopyN(guidAsWideChar, guidAsUnicodeString.Buffer, 39);

                guidAsWideChar[39] = L'\0';

                swprintf(
                    buffer,
                    255,
                    L"\\Registry\\Machine\\BCD00000000\\Objects\\%s\\Elements",
                    guidAsWideChar
                    );

                // Change key permissions to allow us to create sub keys.
                StripPermissions(buffer);

                // Enable test-signing mode.

                String::Concatenate(buffer, L"\\16000049");

                // Change key permissions (if it already exists) to allow us to set values.
                StripPermissions(buffer);

                UnicodeString::Init(&keyName, buffer);
                UnicodeString::Init(&valueName, L"Element");

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
