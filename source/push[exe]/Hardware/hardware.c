#include <sl.h>
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
    DWORD bus = 0, dev = 0, func = 0;
    DWORD count = 0;
    DWORD pciAddress = 0xFFFFFFFF;
    DWORD conf[3] = {0};
    DWORD error = 0;    
    BOOLEAN multiFuncFlag = FALSE;
    BYTE type = 0;

    if(R0DriverHandle == INVALID_HANDLE_VALUE)
    {
        return 0xFFFFFFFF;
    }

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
    UNICODE_STRING *pstrDeviceName,
    DWORD* pdwNumberOfPhysicalMonitors
    );

typedef NTSTATUS(__stdcall* TYPE_GetPhysicalMonitors)(
    UNICODE_STRING *pstrDeviceName,
    DWORD dwPhysicalMonitorArraySize,
    DWORD *pdwNumPhysicalMonitorHandlesInArray,
    HANDLE *phPhysicalMonitorArray
    );

typedef NTSTATUS(__stdcall* TYPE_DDCCIGetTimingReport)(
    HANDLE hMonitor,
    LPMC_TIMING_REPORT pmtr
    );

typedef NTSTATUS(__stdcall* TYPE_DDCCIGetVCPFeature)(
    HANDLE hMonitor,
    DWORD dwVCPCode,
    LPMC_VCP_CODE_TYPE pvct,
    DWORD *pdwCurrentValue,
    DWORD *pdwMaximumValue
    );

typedef NTSTATUS (__stdcall* TYPE_DDCCISetVCPFeature)(
    HANDLE hMonitor,
    DWORD dwVCPCode,
    DWORD dwNewValue
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
    HANDLE      hMonitor,
    MONITORINFOEXW* lpmi
    );

typedef INTBOOL(__stdcall* MONITORENUMPROC)(HMONITOR, HDC, LPRECT, LPARAM);

INTBOOL __stdcall EnumDisplayMonitors(
    HANDLE hdc,
    RECT* lprcClip,
    MONITORENUMPROC lpfnEnum,
    DWORD dwData
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
    int nIndex
    );

#define SM_CXSCREEN     0
#define SM_CYSCREEN     1
#define SM_CMONITORS    80

int MonitorWidth;
int MonitorHeight;
CORE_LIST coreList;


VOID GetHardwareInfo()
{
    int i = 0;
    int monitorCount;
    CORE_LIST *coreListEntry;
    HANDLE gdi32;
    
    //use 64 bits to force allmul() on 32 bit builds
    UINT64 physicalPages;
    UINT64 pageSize;

    InitGpuHardware();
    GPU_Initialize(PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress);

    PushSharedMemory->HarwareInformation.DisplayDevice.FrameBuffer.Total = GPU_GetTotalMemory();
    PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax = GPU_GetMaximumEngineClock();
    PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax = GPU_GetMaximumMemoryClock();
    PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax = GPU_GetMaximumVoltage();

    if (SlIniReadBoolean(L"Settings", L"GpuUsageD3DKMT", FALSE))
        PushGpuLoadD3DKMT = TRUE;

    // Get the number of processors in the system
    NtQuerySystemInformation(SystemBasicInformation, &HwInfoSystemBasicInformation, sizeof(SYSTEM_BASIC_INFORMATION), 0);

    PushSharedMemory->HarwareInformation.Processor.NumberOfCores = HwInfoSystemBasicInformation.NumberOfProcessors;
    
    physicalPages = HwInfoSystemBasicInformation.NumberOfPhysicalPages;
    pageSize = HwInfoSystemBasicInformation.PageSize;

    //byte => megabytes
    PushSharedMemory->HarwareInformation.Memory.Total = (physicalPages * pageSize) / 1048576;
    
    coreListEntry = &coreList;

    for (i = 0; i < PushSharedMemory->HarwareInformation.Processor.NumberOfCores; i++)
    {
        coreListEntry->nextEntry = (CORE_LIST*)Memory_Allocate(
                                    /*PushHeapHandle,
                                    0,*/
                                    sizeof(CORE_LIST)
                                    );

        coreListEntry->number                   = i;
        coreListEntry->idleTime.QuadPart        = 0;
        coreListEntry->perfCounter.QuadPart     = 0;
        coreListEntry->usage                    = 0.0f;

        coreListEntry = coreListEntry->nextEntry;
    }

    CPU_Intialize();

    gdi32 = Module_Load(L"gdi32.dll");

    GetNumberOfPhysicalMonitors = Module_GetProcedureAddress(gdi32, "GetNumberOfPhysicalMonitors");
    GetPhysicalMonitors = Module_GetProcedureAddress(gdi32, "GetPhysicalMonitors");
    DDCCIGetTimingReport = Module_GetProcedureAddress(gdi32, "DDCCIGetTimingReport");
    DDCCIGetVCPFeature = Module_GetProcedureAddress(gdi32, "DDCCIGetVCPFeature");
    DDCCISetVCPFeature = Module_GetProcedureAddress(gdi32, "DDCCISetVCPFeature");

    monitorCount = GetSystemMetrics(SM_CMONITORS);
    MonitorWidth = GetSystemMetrics(SM_CXSCREEN);
    MonitorHeight = GetSystemMetrics(SM_CYSCREEN);

    MonitorHandles = Memory_Allocate(sizeof(HANDLE) * monitorCount);

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
    MC_TIMING_REPORT timingReport;

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
    
    if (DiskMonitorInitialized)
        PushSharedMemory->HarwareInformation.Disk.ReadWriteRate             = GetDiskReadWriteRate();

    DDCCIGetTimingReport(MonitorHandles[1], &timingReport);

    PushSharedMemory->HarwareInformation.Display.RefreshRate = timingReport.dwVerticalFrequencyInHZ / 100;
}


typedef DWORD (__stdcall *TYPE_CM_Get_Device_Interface_List_ExW)(
    GUID *InterfaceClassGuid,
    WCHAR* pDeviceID,
    WCHAR* Buffer,
    ULONG BufferLen,
    ULONG ulFlags,
    HANDLE hMachine
    );

TYPE_CM_Get_Device_Interface_List_ExW   CM_Get_Device_Interface_List_ExW;

GUID GUID_DISPLAY_DEVICE_ARRIVAL_I = { 0x1ca05180, 0xa699, 0x450a, { 0x9a, 0x0c, 0xde, 0x4f, 0xbe, 0x3d, 0xdd, 0x89 } };


VOID GetDisplayAdapterDevicePath( WCHAR* Buffer )
{
    HANDLE cfgmgr32;

    cfgmgr32 = Module_Load(L"cfgmgr32.dll");

    CM_Get_Device_Interface_List_ExW = (TYPE_CM_Get_Device_Interface_List_ExW)
        Module_GetProcedureAddress(cfgmgr32, "CM_Get_Device_Interface_List_ExW");

    CM_Get_Device_Interface_List_ExW(&GUID_DISPLAY_DEVICE_ARRIVAL_I, NULL, Buffer, 213, 0, NULL);
}
