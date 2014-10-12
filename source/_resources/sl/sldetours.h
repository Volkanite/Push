class SlHookManager
{
public:
    VOID* DetourFunction( BYTE* orig, BYTE *det );
    VOID* DetourApi( WCHAR* dllName, CHAR* apiName, BYTE *det );
};