#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS __stdcall NtCreateKey(
    HANDLE* KeyHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes,
    ULONG TitleIndex,
    PUNICODE_STRING Class,
    ULONG CreateOptions,
    ULONG* Disposition
    );

NTSTATUS __stdcall NtOpenKey(
    HANDLE* KeyHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes
    );

NTSTATUS __stdcall NtSetValueKey(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    ULONG TitleIndex,
    ULONG Type,
    VOID* Data,
    ULONG DataSize
    );

#ifdef __cplusplus
}
#endif
