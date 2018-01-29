typedef enum _SECTION_INHERIT
{
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

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
    
NTSTATUS __stdcall NtMapViewOfSection(
    VOID* SectionHandle,
    VOID* ProcessHandle,
    VOID** BaseAddress,
    UINT_B ZeroBits,
    UINT_B CommitSize,
    LARGE_INTEGER* SectionOffset,
    SIZE_T* ViewSize,
    SECTION_INHERIT InheritDisposition,
    ULONG AllocationType,
    ULONG Win32Protect
    );

#ifdef __cplusplus
}
#endif
