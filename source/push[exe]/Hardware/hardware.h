#include <disk.h>


#define REGISTER_VENDORID   0x00
#define REGISTER_CLASSCODE  0x08
#define REGISTER_BAR0       0x10
#define REGISTER_BAR2       0x18



#define NVIDIA  0x10DE
#define AMD     0x1002
#define INTEL   0x8086

VOID RefreshHardwareInfo(
    );

VOID GetHardwareInfo(
    );

DWORD ReadGpuRegister(
    DWORD dwAddr
    );

VOID Hardware_ForceMaxClocks(
    );

VOID AddToThreadList(
    UINT16 threadID
    );

VOID RemoveThreadEntry(
    UINT16 threadID
    );

UINT16 GetDiskResponseTime(UINT32 ProcessId);
VOID GetDisplayAdapterDevicePath(WCHAR* Buffer);
void SetBrightness(int Brightness);


