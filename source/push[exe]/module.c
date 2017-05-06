#include <sl.h>


VOID* Module_Load( WCHAR* FileName )
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


VOID* Module_GetHandle( WCHAR* FileName )
{
    UNICODE_STRING moduleName;
    VOID* moduleHandle;

    UnicodeString_Init(&moduleName, FileName);
    LdrGetDllHandle(NULL, NULL, &moduleName, &moduleHandle);

    return moduleHandle;
}


#include <string.h>
VOID RtlInitAnsiString( PANSI_STRING DestinationString, CHAR* SourceString )
{
    if (SourceString)
        DestinationString->MaximumLength = (DestinationString->Length = (UINT16)strlen(SourceString)) + 1;
    else
        DestinationString->MaximumLength = DestinationString->Length = 0;

    DestinationString->Buffer = SourceString;
}


VOID* Module_GetProcedureAddress( VOID* DllHandle, CHAR* ProcedureName )
{
    ANSI_STRING procedureName;
    VOID* procedureAddress = NULL;

    RtlInitAnsiString(&procedureName, ProcedureName);
    LdrGetProcedureAddress(DllHandle, &procedureName, 0, &procedureAddress);

    return procedureAddress;
}
