extern "C" 
NTSTATUS SlLoadDriver( 
    _In_ WCHAR* ServiceName, 
    _In_ WCHAR* DriverPath, 
    _In_ WCHAR* DisplayName, 
    _In_ WCHAR* DeviceName, 
    _In_ BOOLEAN Filter, 
    _Out_ VOID** DriverHandle
    );