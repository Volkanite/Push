#include <sl.h>
#include <push.h>


HANDLE HeapHandle;
UINT32 BytesAllocated;


SIZE_T __stdcall RtlSizeHeap(
    VOID* HeapHandle,
    ULONG Flags,
    VOID* BaseAddress
    );

NTSTATUS __stdcall NtOpenDirectoryObject(VOID**  FileHandle,
    DWORD   DesiredAccess,
    OBJECT_ATTRIBUTES*  ObjectAttributes
    );

#define SEC_COMMIT   0x8000000
#define HEAP_GENERATE_EXCEPTIONS        0x00000004

#define DIRECTORY_QUERY                 0x0001
#define DIRECTORY_TRAVERSE              0x0002
#define DIRECTORY_CREATE_OBJECT         0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY   0x0008


VOID* BaseNamedObjectDirectory = NULL;


VOID* BaseGetNamedObjectDirectory()
{
    OBJECT_ATTRIBUTES objAttrib;
    UNICODE_STRING bnoString;
    LONG Status;
    WCHAR baseNamedObjectDirectoryName[29];

    String_Format(
        baseNamedObjectDirectoryName,
        29,
        L"\\Sessions\\%u\\BaseNamedObjects",
        NtCurrentTeb()->ProcessEnvironmentBlock->SessionId
        );

    UnicodeString_Init(&bnoString, baseNamedObjectDirectoryName);

    if (!BaseNamedObjectDirectory)
    {

        objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
        objAttrib.RootDirectory = NULL;
        objAttrib.Attributes = OBJ_CASE_INSENSITIVE;
        objAttrib.ObjectName = &bnoString;
        objAttrib.SecurityDescriptor = NULL;
        objAttrib.SecurityQualityOfService = NULL;

        Status = NtOpenDirectoryObject(
            &BaseNamedObjectDirectory,
            DIRECTORY_CREATE_OBJECT |
            DIRECTORY_CREATE_SUBDIRECTORY |
            DIRECTORY_QUERY |
            DIRECTORY_TRAVERSE,
            &objAttrib
            );
    }

    return BaseNamedObjectDirectory;
}


VOID* Memory_MapViewOfSection( WCHAR* FileName, DWORD Size, HANDLE* SectionHandle )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    OBJECT_ATTRIBUTES objAttrib = {0};
    UNICODE_STRING sectionName;
    LARGE_INTEGER sectionSize;
    VOID* viewBase;

    UnicodeString_Init(&sectionName, FileName);

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = BaseGetNamedObjectDirectory();
    objAttrib.Attributes = OBJ_OPENIF;
    objAttrib.ObjectName = &sectionName;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    sectionSize.u.LowPart = Size;
    sectionSize.u.HighPart = 0;

    status = NtCreateSection(
        &sectionHandle,
        STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_QUERY,
        &objAttrib,
        &sectionSize,
        PAGE_READWRITE,
        SEC_COMMIT,
        NULL
        );

    *SectionHandle = sectionHandle;

    //must be NULL or will fail
    viewBase = NULL;

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &viewBase,
        0,
        0,
        NULL,
        &sectionSize.u.LowPart,
        ViewShare,
        0,
        PAGE_READWRITE
        );

    return viewBase;
}


VOID* Memory_Allocate( UINT_B Size )
{
#if DEBUG
    //BytesAllocated += Size;

    //Log(L"Allocating %i bytes, BytesAllocated: %i", Size, BytesAllocated);
#endif

    if (!HeapHandle)
        HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    return RtlAllocateHeap(HeapHandle, 0, Size);
}


VOID* Memory_AllocateEx( UINT_B Size, DWORD Flags )
{
    BytesAllocated += Size;

    if (!HeapHandle)
        HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;


    return RtlAllocateHeap(HeapHandle, Flags, Size);
}


VOID* Memory_ReAllocate( VOID* Memory, SIZE_T Size )
{
    if (!HeapHandle)
        HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    return RtlReAllocateHeap(HeapHandle, HEAP_GENERATE_EXCEPTIONS, Memory, Size);
}


VOID Memory_Free( VOID* Heap )
{
    if (!Heap)
    {
        //Log(L"Why are you trying to free memory that doesn't exist!?");

        return;
    }

#if DEBUG
    //SIZE_T size;

    //size = RtlSizeHeap(PushHeapHandle, 0, Heap);

    //Log(L"Freeing %i bytes of memory", size);

    //BytesAllocated -= size;
#endif

    if (!HeapHandle)
        HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    RtlFreeHeap(HeapHandle, 0, Heap);
}


VOID Memory_Copy( VOID* Destination, VOID* Source, UINT32 Length )
{
    ntdll_memcpy(Destination, Source, Length);
}


VOID Memory_Clear( VOID* Region, UINT32 Size )
{
    Memory_ClearEx(Region, 0, Size);
}


VOID Memory_ClearEx(VOID* Region, DWORD Val, UINT32 Size)
{
    ntdll_memset(Region, Val, Size);
}


WCHAR* Memory_FindFirstChar(const WCHAR *ptr, WCHAR ch, UINT32 n)
{
    const WCHAR *end;
    for (end = ptr + n; ptr < end; ptr++)
        if (*ptr == ch)
            return (WCHAR *)(UINT_B)ptr;
    return NULL;
}
