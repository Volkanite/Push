#include <sl.h>
#include "push.h"


HANDLE Module_Load( WCHAR* FileName )
{
    UNICODE_STRING moduleName;
    VOID *moduleHandle;
    NTSTATUS status;

    UnicodeString_Init(&moduleName, FileName);

    status = LdrLoadDll(NULL, NULL, &moduleName, &moduleHandle);

    if (!NT_SUCCESS(status))
        return NULL;
    else
        return moduleHandle;
}


NTSTATUS __stdcall LdrGetDllHandle(
    WCHAR* DllPath,
    ULONG* DllCharacteristics,
    UNICODE_STRING* DllName,
    VOID** DllHandle
    );

NTSTATUS __stdcall LdrGetProcedureAddress(
    VOID* DllHandle,
    PANSI_STRING ProcedureName,
    ULONG ProcedureNumber,
    VOID** ProcedureAddress
    );


HANDLE Module_GetHandle( WCHAR* FileName )
{
    UNICODE_STRING moduleName;
    VOID* moduleHandle;

    UnicodeString_Init(&moduleName, FileName);
    LdrGetDllHandle(NULL, NULL, &moduleName, &moduleHandle);

    return moduleHandle;
}

UINT32 strlen_o(CHAR* String)
{
    const char *p;

    if (!String)
        return NULL;

    p = String;

    while (*p)
        p++;

    return p - String;
}


VOID RtlInitAnsiString( PANSI_STRING DestinationString, CHAR* SourceString )
{
    if (SourceString)
        DestinationString->MaximumLength = (DestinationString->Length = (UINT16)strlen ? strlen(SourceString) : strlen_o(SourceString)) + 1;
    else
        DestinationString->MaximumLength = DestinationString->Length = 0;

    DestinationString->Buffer = SourceString;
}


HANDLE Module_GetProcedureAddress( HANDLE DllHandle, CHAR* ProcedureName )
{
    ANSI_STRING procedureName;
    VOID* procedureAddress = NULL;

    RtlInitAnsiString(&procedureName, ProcedureName);
    LdrGetProcedureAddress(DllHandle, &procedureName, 0, &procedureAddress);

    return procedureAddress;
}
