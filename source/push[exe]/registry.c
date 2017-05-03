#include <sl.h>


VOID* Registry_OpenKey(
    WCHAR* KeyName,
    DWORD DesiredAccess
    )
{
    UNICODE_STRING keyName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE keyHandle = NULL;

    UnicodeString_Init(&keyName, KeyName);

    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = &keyName;
    objectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;

    NtOpenKey(
        &keyHandle,
        DesiredAccess,
        &objectAttributes
        );

    return keyHandle;
}
