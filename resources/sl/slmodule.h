class Module
{
public:
    static VOID* Load( WCHAR* ModuleName );
    static VOID* GetProcedureAddress( VOID* DllHandle, CHAR* ProcedureName );
};