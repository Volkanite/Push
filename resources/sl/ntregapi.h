#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS __stdcall NtCreateKey(
    _Out_ HANDLE* KeyHandle,
    _In_ DWORD DesiredAccess,
    _In_ OBJECT_ATTRIBUTES* ObjectAttributes,
    _In_ ULONG TitleIndex,
    _In_ PUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _Out_ ULONG* Disposition
    );
    
NTSTATUS __stdcall NtOpenKey(
    _Out_ HANDLE* KeyHandle,
    _In_ DWORD DesiredAccess,
    _In_ OBJECT_ATTRIBUTES* ObjectAttributes
    );
    
NTSTATUS __stdcall NtSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ ULONG TitleIndex,
    _In_ ULONG Type,
    _In_ VOID* Data,
    _In_ ULONG DataSize
    );
    
#ifdef __cplusplus
}
#endif
