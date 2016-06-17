#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS __stdcall NtProtectVirtualMemory(
    HANDLE ProcessHandle,
    VOID** BaseAddress,
    SIZE_T* RegionSize,
    ULONG NewProtect,
    ULONG* OldProtect
    );
    
NTSTATUS __stdcall NtCreateSection(
    VOID** SectionHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes,
    LARGE_INTEGER* MaximumSize,
    ULONG SectionPageProtection,
    ULONG AllocationAttributes,
    VOID* FileHandle
    );

#ifdef __cplusplus
}
#endif
