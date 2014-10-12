#ifdef __cplusplus
extern "C" {
#endif

VOID* SlLoadDriver( 
    WCHAR* ServiceName, 
    WCHAR* BinaryName, 
    WCHAR* DisplayName, 
    WCHAR* DeviceName,
    BOOLEAN Filter
    );

#ifdef __cplusplus
}
#endif
