VOID* Memory_Allocate(UINT_B Size);
VOID* Memory_AllocateEx(UINT_B Size, DWORD Flags);
VOID* Memory_ReAllocate(VOID* Memory, SIZE_T Size);
VOID Memory_Free(VOID* Base);
VOID Memory_Copy(VOID* Destination, VOID* Source, UINT32 Length);
VOID Memory_Clear(VOID* Region, UINT32 Size);
VOID Memory_ClearEx(VOID* Region, DWORD Val, UINT32 Size);
VOID* Memory_MapViewOfSection(WCHAR* FileName, DWORD Size, HANDLE* SectionHandle);
WCHAR* Memory_FindFirstChar(const WCHAR *ptr, WCHAR ch, UINT32 n);
