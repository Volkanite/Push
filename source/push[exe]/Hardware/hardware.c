#include <sl.h>
#include <string.h>
#include <push.h>
#include <ring0.h>

#include "CPU\cpu.h"
#include "disk.h"
#include "hardware.h"
#include "GPU\gpu.h"


UINT64                      g_hNVPMContext;
BOOLEAN                     PushGpuLoadD3DKMT = FALSE;
DWORD                       dwMappedMemAddr;
//PUSH_HARDWARE_INFORMATION   hardware;
SYSTEM_BASIC_INFORMATION    HwInfoSystemBasicInformation;


UINT8 GetCpuTemp()
{
    return CPU_GetTemperature();
}


UINT64
D3DKMTGetMemoryUsage();


UINT32 GetFrameBufferSize()
{
    return GPU_GetTotalMemory();
}


//in megabytes
UINT32 GetRamUsed()
{
    SYSTEM_BASIC_PERFORMANCE_INFORMATION performanceInfo;
    UINT64 committedPages; //force allmul() on x86

    NtQuerySystemInformation(
        SystemBasicPerformanceInformation, 
        &performanceInfo, 
        sizeof(SYSTEM_BASIC_PERFORMANCE_INFORMATION), 
        NULL
        );

    committedPages = performanceInfo.CommittedPages;

    return (committedPages * HwInfoSystemBasicInformation.PageSize) / 1048576; //byte => megabytes
}


//percentage (non-float)
UINT8 GetRamUsage()
{
    return 100 * ((float)PushSharedMemory->HarwareInformation.Memory.Used / (float)PushSharedMemory->HarwareInformation.Memory.Total);
}


typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef struct _PH_UINT64_DELTA
{
    UINT64 Value;
    UINT64 Delta;
} PH_UINT64_DELTA, *PPH_UINT64_DELTA;


#define PhUpdateDelta(DltMgr, NewValue) \
    ((DltMgr)->Delta = (NewValue) - (DltMgr)->Value, \
    (DltMgr)->Value = (NewValue), (DltMgr)->Delta)

FLOAT PhCpuKernelUsage;
FLOAT PhCpuUserUsage;
FLOAT *PhCpusKernelUsage;
FLOAT* PhCpusUserUsage;

PPH_UINT64_DELTA PhCpusKernelDelta;
PPH_UINT64_DELTA PhCpusUserDelta;
PPH_UINT64_DELTA PhCpusIdleDelta;

PH_UINT64_DELTA PhCpuKernelDelta;
PH_UINT64_DELTA PhCpuUserDelta;
PH_UINT64_DELTA PhCpuIdleDelta;


SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *PhCpuInformation;


#define SUCCEEDED(hr) (((long)(hr)) >= 0)


UINT32
GetDiskReadWriteRate()
{
    return DiskGetBytesDelta();
}


UINT16 GetDiskResponseTime( UINT32 ProcessId )
{
    return DiskGetResponseTime(ProcessId);
}


VOID Hardware_ForceMaxClocks()
{
    GPU_ForceMaximumClocks();
}


DWORD FindPciDeviceByClass( BYTE baseClass, BYTE subClass, BYTE programIf, BYTE index )
{
    if(R0DriverHandle == INVALID_HANDLE_VALUE)
    {
        return 0xFFFFFFFF;
    }

    DWORD bus = 0, dev = 0, func = 0;
    DWORD count = 0;
    DWORD pciAddress = 0xFFFFFFFF;
    DWORD conf[3] = {0};
    DWORD error = 0;    
    BOOLEAN multiFuncFlag = FALSE;
    BYTE type = 0;
    count = 0;

    for(bus = 0; bus <= 255; bus++)
    {
        for(dev = 0; dev < 31; dev++)
        {
            multiFuncFlag = FALSE;
            for(func = 0; func < 7; func++)
            {
                if(multiFuncFlag == FALSE && func > 0)
                {
                    break;
                }
                pciAddress = PciBusDevFunc(bus, dev, func);
                if (R0ReadPciConfig(pciAddress, 0, (BYTE*)conf, sizeof(conf)))
                {
                    if(func == 0) // Is Multi Function Device
                    { 
                        if (R0ReadPciConfig(pciAddress, 0x0E, (BYTE*)&type, sizeof(type)))
                        {
                            if(type & 0x80)
                            {
                                multiFuncFlag = TRUE;
                            }
                        }
                    }
                    if((conf[2] & 0xFFFFFF00) == 
                            (((DWORD)baseClass << 24) |
                            ((DWORD)subClass << 16) |
                            ((DWORD)programIf << 8))
                        )
                    {
                        if (count == index)
                        {
                            return pciAddress;
                        }

                        count++;
                        continue;
                    }
                }
            }
        }
    }
    return 0xFFFFFFFF;
}


VOID* HwMmio;


DWORD ReadGpuRegister( DWORD Address )
{
    DWORD* val=0;
    DWORD address;
    DWORD ret;

    if (!HwMmio) return 0;

    address = ((DWORD)HwMmio) + Address;

    val = (DWORD*)address;
    ret = *val;
    return ret;
}


DWORD
GetBarAddress( DWORD Bar )
{
    DWORD barAddress;

    R0ReadPciConfig(
        PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress,
        Bar,
        (BYTE *)&barAddress,
        sizeof(barAddress)
        );

    // 16-Byte alligned address;
    return (barAddress & 0x0fffffff0);
}


UINT32
GetBarSize(
    DWORD BarAddress
    )
{
   unsigned pos = 0;

   while (!(BarAddress & 1))
   {
      BarAddress >>= 1;
      ++pos;
   }

    return 1 << (pos - 1);
}


VOID InitGpuHardware()
{
    DWORD bar;

    PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress = FindPciDeviceByClass(0x03, 0x00, 0x00, 0);

    R0ReadPciConfig(
        PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress,
        REGISTER_VENDORID,
        (BYTE *)&PushSharedMemory->HarwareInformation.DisplayDevice.VendorId,
        sizeof(PushSharedMemory->HarwareInformation.DisplayDevice.VendorId)
        );

    if (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId == NVIDIA)
        bar = REGISTER_BAR0;
    else if (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId == AMD)
        bar = REGISTER_BAR2;
    else
        return;

    PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress = GetBarAddress(bar);

    HwMmio = R0MapPhysicalMemory(
        PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress,
        GetBarSize(PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress)
                );
}

typedef struct _MC_TIMING_REPORT
{
    DWORD dwHorizontalFrequencyInHZ;
    DWORD dwVerticalFrequencyInHZ;
    BYTE bTimingStatusByte;

} MC_TIMING_REPORT, *LPMC_TIMING_REPORT;

typedef enum _MC_VCP_CODE_TYPE {
    MC_MOMENTARY,
    MC_SET_PARAMETER
} MC_VCP_CODE_TYPE, *LPMC_VCP_CODE_TYPE;


typedef NTSTATUS(__stdcall* TYPE_GetNumberOfPhysicalMonitors)(
    _In_   UNICODE_STRING *pstrDeviceName,
    _Out_  DWORD* pdwNumberOfPhysicalMonitors
    );

typedef NTSTATUS(__stdcall* TYPE_GetPhysicalMonitors)(
    _In_   UNICODE_STRING *pstrDeviceName,
    _In_   DWORD dwPhysicalMonitorArraySize,
    _Out_  DWORD *pdwNumPhysicalMonitorHandlesInArray,
    _Out_  HANDLE *phPhysicalMonitorArray
    );

typedef NTSTATUS(__stdcall* TYPE_DDCCIGetTimingReport)(
    _In_   HANDLE hMonitor,
    _Out_  LPMC_TIMING_REPORT pmtr
    );

typedef NTSTATUS(__stdcall* TYPE_DDCCIGetVCPFeature)(
    _In_       HANDLE hMonitor,
    _In_       DWORD dwVCPCode,
    _Out_opt_  LPMC_VCP_CODE_TYPE pvct,
    _Out_      DWORD *pdwCurrentValue,
    _Out_opt_  DWORD *pdwMaximumValue
    );

typedef NTSTATUS (__stdcall* TYPE_DDCCISetVCPFeature)(
    _In_  HANDLE hMonitor,
    _In_  DWORD dwVCPCode,
    _In_  DWORD dwNewValue
    );


TYPE_GetNumberOfPhysicalMonitors    GetNumberOfPhysicalMonitors;
TYPE_GetPhysicalMonitors            GetPhysicalMonitors;
TYPE_DDCCIGetTimingReport           DDCCIGetTimingReport;
TYPE_DDCCIGetVCPFeature             DDCCIGetVCPFeature;
TYPE_DDCCISetVCPFeature             DDCCISetVCPFeature;


HANDLE* MonitorHandles;


#define CCHDEVICENAME 32
typedef struct tagMONITORINFOEXW {
    DWORD cbSize;
    RECT  rcMonitor;
    RECT  rcWork;
    DWORD dwFlags;
    WCHAR szDevice[CCHDEVICENAME];
} MONITORINFOEXW, *LPMONITORINFOEXW;
INTBOOL __stdcall GetMonitorInfoW(
    _In_  HANDLE      hMonitor,
    _Out_ MONITORINFOEXW* lpmi
    );

typedef INTBOOL(__stdcall* MONITORENUMPROC)(HMONITOR, HDC, LPRECT, LPARAM);

INTBOOL __stdcall EnumDisplayMonitors(
    _In_opt_ HANDLE hdc,
    _In_opt_ RECT* lprcClip,
    _In_ MONITORENUMPROC lpfnEnum,
    _In_ DWORD dwData
    );


INTBOOL __stdcall MonitorEnumProc( HANDLE hMonitor, HANDLE hdcMonitor, RECT* lprcMonitor, DWORD dwData )
{
    DWORD monitorCount;
    MONITORINFOEXW monitorInformation;
    UNICODE_STRING deviceName;
    DWORD numPhysicalMonitorHandlesInArray;

    static int monEnums = 0;

    monitorInformation.cbSize = sizeof(MONITORINFOEXW);

    GetMonitorInfoW(hMonitor, &monitorInformation);

    deviceName.Buffer = monitorInformation.szDevice;
    deviceName.Length = 24;
    deviceName.MaximumLength = 26;

    GetNumberOfPhysicalMonitors(&deviceName, &monitorCount);

    numPhysicalMonitorHandlesInArray = 0;

    GetPhysicalMonitors(&deviceName, monitorCount, &numPhysicalMonitorHandlesInArray, &MonitorHandles[monEnums++]);

    return TRUE;
}

int __stdcall GetSystemMetrics(
    _In_ int nIndex
    );
#define SM_CMONITORS            80
VOID GetHardwareInfo()
{
    int i = 0;
    CORE_LIST *coreListEntry;
    
    //use 64 bits to force allmul() on 32 bit builds
    UINT64 physicalPages;
    UINT64 pageSize;

    InitGpuHardware();
    GPU_Initialize(PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress);

    PushSharedMemory->HarwareInformation.DisplayDevice.FrameBuffer.Total = GPU_GetTotalMemory();
    PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax = GPU_GetMaximumEngineClock();
    PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax = GPU_GetMaximumMemoryClock();
    PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax = GPU_GetMaximumVoltage();

    if (SlIniReadBoolean(L"Settings", L"GpuUsageD3DKMT", FALSE, L".\\" PUSH_SETTINGS_FILE))
        PushGpuLoadD3DKMT = TRUE;

    // Get the number of processors in the system
    NtQuerySystemInformation(SystemBasicInformation, &HwInfoSystemBasicInformation, sizeof(SYSTEM_BASIC_INFORMATION), 0);

    PushSharedMemory->HarwareInformation.Processor.NumberOfCores = HwInfoSystemBasicInformation.NumberOfProcessors;
    
    physicalPages = HwInfoSystemBasicInformation.NumberOfPhysicalPages;
    pageSize = HwInfoSystemBasicInformation.PageSize;

    //byte => megabytes
    PushSharedMemory->HarwareInformation.Memory.Total = (physicalPages * pageSize) / 1048576;
    
    coreListEntry = &PushSharedMemory->HarwareInformation.Processor.coreList;

    for (i = 0; i < PushSharedMemory->HarwareInformation.Processor.NumberOfCores; i++)
    {
        coreListEntry->nextEntry = (CORE_LIST*)RtlAllocateHeap(
                                    PushHeapHandle,
                                    0,
                                    sizeof(CORE_LIST)
                                    );

        coreListEntry->number                   = i;
        coreListEntry->idleTime.QuadPart        = 0;
        coreListEntry->perfCounter.QuadPart     = 0;
        coreListEntry->usage                    = 0.0f;

        coreListEntry = coreListEntry->nextEntry;
    }
    
    // Start disk monitoring;
    DiskStartMonitoring();

    CPU_Intialize();

    HANDLE gdi32;

    gdi32 = Module_Load(L"gdi32.dll");

    GetNumberOfPhysicalMonitors = Module_GetProcedureAddress(gdi32, "GetNumberOfPhysicalMonitors");
    GetPhysicalMonitors = Module_GetProcedureAddress(gdi32, "GetPhysicalMonitors");
    DDCCIGetTimingReport = Module_GetProcedureAddress(gdi32, "DDCCIGetTimingReport");
    DDCCIGetVCPFeature = Module_GetProcedureAddress(gdi32, "DDCCIGetVCPFeature");
    DDCCISetVCPFeature = Module_GetProcedureAddress(gdi32, "DDCCISetVCPFeature");

    int monitorCount = GetSystemMetrics(SM_CMONITORS);

    MonitorHandles = RtlAllocateHeap(PushHeapHandle, 0, sizeof(HANDLE) * monitorCount);

    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, NULL);
}


#define STATUS_GRAPHICS_DDCCI_VCP_NOT_SUPPORTED ((NTSTATUS)0xC01E0584L)    

#define VCP_BRIGHTNESS              0x10
#define VCP_HORIZONTAL_FREQUENCY    0xAC
#define VCP_VERTICAL_FREQUENCY      0xAE
#define VCP_VERSION                 0xDF
#define VCP_POWER_MODE              0xD6


VOID RefreshHardwareInfo()
{
    GPU_INFO gpuInfo;

    GPU_GetInfo(&gpuInfo);

    PushSharedMemory->HarwareInformation.Processor.Speed                = CPU_GetSpeed();
    //PushSharedMemory->HarwareInformation.Processor.Load                 = GetCpuLoad();
    //PushSharedMemory->HarwareInformation.Processor.MaxCoreUsage         = GetMaxCoreLoad();
    PushSharedMemory->HarwareInformation.Processor.Temperature          = GetCpuTemp();
    PushSharedMemory->HarwareInformation.DisplayDevice.Load             = gpuInfo.Load;
    PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock      = gpuInfo.EngineClock;
    PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClock      = gpuInfo.MemoryClock;
    PushSharedMemory->HarwareInformation.DisplayDevice.Voltage          = gpuInfo.Voltage;
    PushSharedMemory->HarwareInformation.DisplayDevice.Temperature      = gpuInfo.Temperature;
    PushSharedMemory->HarwareInformation.DisplayDevice.FrameBuffer.Used = gpuInfo.MemoryUsed;
    PushSharedMemory->HarwareInformation.DisplayDevice.FrameBuffer.Load = gpuInfo.MemoryUsage;
    PushSharedMemory->HarwareInformation.DisplayDevice.FanSpeed         = gpuInfo.FanSpeed;
    PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle     = gpuInfo.FanDutyCycle;
    PushSharedMemory->HarwareInformation.Memory.Used                    = GetRamUsed();
    PushSharedMemory->HarwareInformation.Memory.Load                    = GetRamUsage();
    PushSharedMemory->HarwareInformation.Disk.ReadWriteRate             = GetDiskReadWriteRate();

    MC_TIMING_REPORT timingReport;

    DDCCIGetTimingReport(MonitorHandles[1], &timingReport);

    PushSharedMemory->HarwareInformation.Display.RefreshRate = timingReport.dwVerticalFrequencyInHZ / 100;
}


#define DIGCF_DEVICEINTERFACE   0x00000010
#define DIGCF_PRESENT           0x00000002

typedef INT32(__stdcall *TYPE_SetupDiDestroyDeviceInfoList)(VOID *DeviceInfoSet);

typedef VOID*(__stdcall *TYPE_SetupDiGetClassDevsW)(
    const GUID* ClassGuid,
    const WCHAR* Enumerator,
    VOID* hwndParent,
    DWORD Flags
    );

typedef INT32(__stdcall *TYPE_SetupDiEnumDeviceInterfaces)(
    VOID                        *DeviceInfoSet,
    SP_DEVINFO_DATA             *DeviceInfoData,
    const GUID                  *InterfaceClassGuid,
    DWORD                       MemberIndex,
    SP_DEVICE_INTERFACE_DATA    *DeviceInterfaceData
    );

typedef INT32(__stdcall *TYPE_SetupDiGetDeviceInterfaceDetailW)(
    VOID                                *DeviceInfoSet,
    SP_DEVICE_INTERFACE_DATA            *DeviceInterfaceData,
    SP_DEVICE_INTERFACE_DETAIL_DATA_W   *DeviceInterfaceDetailData,
    DWORD                               DeviceInterfaceDetailDataSize,
    DWORD                               *RequiredSize,
    SP_DEVINFO_DATA                     *DeviceInfoData
    );


TYPE_SetupDiDestroyDeviceInfoList       SetupDiDestroyDeviceInfoList;
TYPE_SetupDiGetClassDevsW               SetupDiGetClassDevsW;
TYPE_SetupDiEnumDeviceInterfaces        SetupDiEnumDeviceInterfaces;
TYPE_SetupDiGetDeviceInterfaceDetailW   SetupDiGetDeviceInterfaceDetailW;

GUID GUID_DISPLAY_DEVICE_ARRIVAL_I = { 0x1ca05180, 0xa699, 0x450a, { 0x9a, 0x0c, 0xde, 0x4f, 0xbe, 0x3d, 0xdd, 0x89 } };


VOID GetDisplayAdapterDevicePath(WCHAR* Buffer)
{
    VOID *deviceInfoSet;
    UINT32 memberIndex;
    UINT32 detailDataSize;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detailData;
    SP_DEVINFO_DATA deviceInfoData;
    UINT32 result;

    HANDLE setupapi = NULL;

    setupapi = Module_Load(L"setupapi.dll");

    SetupDiGetClassDevsW = (TYPE_SetupDiGetClassDevsW)
        Module_GetProcedureAddress(setupapi, "SetupDiGetClassDevsW");

    SetupDiDestroyDeviceInfoList = (TYPE_SetupDiDestroyDeviceInfoList)
        Module_GetProcedureAddress(setupapi, "SetupDiDestroyDeviceInfoList");

    SetupDiEnumDeviceInterfaces = (TYPE_SetupDiEnumDeviceInterfaces)
        Module_GetProcedureAddress(setupapi, "SetupDiEnumDeviceInterfaces");

    SetupDiGetDeviceInterfaceDetailW = (TYPE_SetupDiGetDeviceInterfaceDetailW)
        Module_GetProcedureAddress(setupapi, "SetupDiGetDeviceInterfaceDetailW");

    deviceInfoSet = SetupDiGetClassDevsW(&GUID_DISPLAY_DEVICE_ARRIVAL_I, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (!deviceInfoSet)
    {
        return;
    }

    memberIndex = 0;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &GUID_DISPLAY_DEVICE_ARRIVAL_I, memberIndex, &deviceInterfaceData))
    {
        detailDataSize = 0x100;
        detailData = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)Memory_Allocate(detailDataSize);
        detailData->cbSize = 6; /*sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)*/
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        result = SetupDiGetDeviceInterfaceDetailW(
            deviceInfoSet,
            &deviceInterfaceData,
            detailData,
            detailDataSize,
            &detailDataSize,
            &deviceInfoData
            );

        if (result)
        {
            String_Copy(Buffer, detailData->DevicePath);
        }

        Memory_Free(detailData);

        memberIndex++;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
}
