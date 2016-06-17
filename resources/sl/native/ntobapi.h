#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS __stdcall NtClose( 
    HANDLE Handle
    );
    
NTSTATUS __stdcall NtSetSecurityObject(
    HANDLE Handle,
    ULONG SecurityInformation,
    VOID* SecurityDescriptor
    );
    
 #ifdef __cplusplus
 }
 #endif
