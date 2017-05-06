VOID* Module_Load(WCHAR* ModuleName);
VOID* Module_GetHandle(WCHAR* FileName);
VOID* Module_GetProcedureAddress(VOID* DllHandle, CHAR* ProcedureName);