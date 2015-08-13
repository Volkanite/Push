class Memory
{
public:
    static VOID* Allocate( UINT_B Size );
    static VOID* Memory::ReAllocate( VOID* Memory, SIZE_T Size );
    static VOID Free( VOID* Base );
    static VOID Copy( VOID* Destination, VOID* Source, UINT32 Length );
    static VOID Clear( VOID* Region, UINT32 Size );
    static VOID* CreateFileMapping( WCHAR* FileName, DWORD Size );
    static WCHAR* FindFirstChar(const WCHAR *ptr, WCHAR ch, UINT32 n);
};
