#ifdef __cplusplus
extern "C" PUSH_HARDWARE_INFORMATION hardware;
#else
extern PUSH_HARDWARE_INFORMATION hardware;
#endif




#define NVIDIA  0x10DE
#define AMD     0x1002
#define INTEL   0x8086


#ifdef __cplusplus
extern "C" {
#endif

VOID RefreshHardwareInfo(
    );

VOID GetHardwareInfo(
    );

DWORD ReadGpuRegister(
    DWORD dwAddr
    );

VOID Hardware_ForceMaxClocks(
    );

#ifdef __cplusplus
}
#endif


VOID AddToThreadList(
    UINT16 threadID
    );

VOID RemoveThreadEntry(
    UINT16 threadID
    );
