#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS SlLoadDriver(
    WCHAR* ServiceName,
    WCHAR* DriverBinaryName,
    WCHAR* DisplayName,
    WCHAR* DeviceName,
    BOOLEAN Filter,
    HANDLE* DriverHandle
    );

#ifdef __cplusplus
}
#endif
