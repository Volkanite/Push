#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS SlLoadDriver( 
    _In_ WCHAR* ServiceName, 
    _In_ WCHAR* DriverBinaryName, 
    _In_ WCHAR* DisplayName, 
    _In_ WCHAR* DeviceName, 
    _In_ BOOLEAN Filter, 
    _Out_ HANDLE* DriverHandle 
    );

#ifdef __cplusplus
}
#endif