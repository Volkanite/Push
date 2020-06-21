#include <sl.h>
#include <push.h>
#include <ring0.h>

#include "CPU\cpu.h"
#include "disk.h"
#include "hardware.h"
#include "GPU\gpu.h"
#include "CPU\os_cpu.h"
#include "wr0.h"


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

    if(gHandle == INVALID_HANDLE_VALUE)
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

                if (Wr0ReadPciConfig(pciAddress, 0, (BYTE*)conf, sizeof(conf)))
                {
                    if(func == 0) // Is Multi Function Device
                    {
                        if (Wr0ReadPciConfig(pciAddress, 0x0E, (BYTE*)&type, sizeof(type)))
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
void **mmioPages;
int numPages;
int pages[] = { 0, 5 };
BOOLEAN PagedMMIO = TRUE;
BOOLEAN MemoryMappedIO;


DWORD ReadGpuRegisterUnpaged( DWORD Address )
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


DWORD ReadGpuRegisterPaged( DWORD Address )
{
    DWORD* val=0;
    DWORD address;
    DWORD ret;
    DWORD someadd;
    DWORD offset;
    int pageNumber;
    int pageIndex;

    pageNumber = Address / 4096;

    for (pageIndex = 0; pageIndex < numPages; pageIndex++)
    {
        if (pageNumber == pages[pageIndex])
            break;
    }

    if (!mmioPages[pageIndex]) return 0;

    someadd = pageNumber * 4096;
    offset = Address - someadd;
    address = ((DWORD)mmioPages[pageIndex]) + offset;
    val = (DWORD*)address;
    ret = *val;
    return ret;
}


DWORD ReadGpuRegisterDirect( DWORD Address )
{
    DWORD buffer;
    DWORD base;
    DWORD physicalAddress;
    //DWORD value;

    base = PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress;

    physicalAddress = base + Address;

    Wr0ReadPhysicalMemory(physicalAddress, (BYTE*) &buffer, 4, sizeof(BYTE));

    return buffer;
}


DWORD ReadGpuRegister( DWORD Address )
{
    if (!PushDriverLoaded && !Wr0DriverLoaded)
        return 0;

    if (MemoryMappedIO)
    {
        if (PagedMMIO)
            return ReadGpuRegisterPaged(Address);
        else
            return ReadGpuRegisterUnpaged(Address);
    }
    else
    {
        return ReadGpuRegisterDirect(Address);
    }
}


DWORD
GetBarAddress( DWORD Bar )
{
    DWORD barAddress;

    Wr0ReadPciConfig(
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

void* gpubios;
VOID File_Dump(WCHAR* FileName, VOID* Addr, UINT32 Size);
VOID InitGpuHardware()
{
    DWORD bar;
    UINT32 size;

    PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress = FindPciDeviceByClass(0x03, 0x00, 0x00, 0);

    Wr0ReadPciConfig(
        PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress,
        REGISTER_VENDORID,
        (BYTE *)&PushSharedMemory->HarwareInformation.DisplayDevice.VendorId,
        sizeof(PushSharedMemory->HarwareInformation.DisplayDevice.VendorId)
        );

    switch (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId)
    {
    case NVIDIA:
    case INTEL:
        bar = REGISTER_BAR0;
        break;
    case AMD:
        bar = REGISTER_BAR2;
        gpubios = R0MapPhysicalMemory(0x000C0000, 63488);
        //File_Dump(L"bios.rom", gpubios, 63488);
        break;
    default:
        return;
        break;
    }

    PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress = GetBarAddress(bar);
    size = GetBarSize(PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress);

    if (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId == INTEL)
    {
        DWORD mchBar = 0xFED10000;

        // 16-Byte alligned address;
        mchBar = (mchBar & 0x0fffffff0);

        PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress = 0xFED10000;

        size = 0x6000;
    }

    if (PushDriverLoaded)
    {
        MemoryMappedIO = TRUE;
    }

    if (MemoryMappedIO)
    {
        if (PagedMMIO)
        {
            int i;

            if (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId == AMD)
            {
                size = 4096;
            }

            numPages = sizeof(pages) / sizeof(pages[0]);

            mmioPages = Memory_Allocate(sizeof(void*)* (sizeof(pages) / sizeof(pages[0])));

            for (i = 0; i < sizeof(pages) / sizeof(pages[0]); i++)
            {
                ULONG address;

                address = PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress;

                if (pages[i] > 0)
                {
                    address = pages[i] * 4096;
                    address += PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress;
                }

                mmioPages[i] = R0MapPhysicalMemory(address, size);
            }
        }
        else
        {
            HwMmio = R0MapPhysicalMemory(PushSharedMemory->HarwareInformation.DisplayDevice.BarAddress, size);
        }
    }
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
#include "..\Hardware\GPU\fan.h"

VOID GetHardwareInfo()
{
    int i = 0;
    int monitorCount;
    HANDLE gdi32;
    int cores = 0;

    //use 64 bits to force allmul() on 32 bit builds
    UINT64 physicalPages;
    UINT64 pageSize;

    InitGpuHardware();
    GPU_Initialize(PushSharedMemory->HarwareInformation.DisplayDevice.pciAddress);

    PushSharedMemory->HarwareInformation.DisplayDevice.FrameBuffer.Total = GPU_GetTotalMemory();
    PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax = GPU_GetMaximumEngineClock();
    PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax = GPU_GetMaximumMemoryClock();
    PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax = GPU_GetMaximumVoltage();

	Log(L"E: %i", PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax);
	Log(L"M: %i", PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax);

	//Only implimented auto-fan control for Nvidia hardware D:
	if (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId == NVIDIA)
		InitializeFanSettings();

    if (Ini_ReadBoolean(L"Settings", L"GpuUsageD3DKMT", FALSE, L".\\" PUSH_SETTINGS_FILE))
        PushGpuLoadD3DKMT = TRUE;

    // Get the number of processors in the system
    NtQuerySystemInformation(SystemBasicInformation, &HwInfoSystemBasicInformation, sizeof(SYSTEM_BASIC_INFORMATION), 0);

    PushSharedMemory->HarwareInformation.Processor.NumberOfThreads = HwInfoSystemBasicInformation.NumberOfProcessors;

    physicalPages = HwInfoSystemBasicInformation.NumberOfPhysicalPages;
    pageSize = HwInfoSystemBasicInformation.PageSize;

    //byte => megabytes
    PushSharedMemory->HarwareInformation.Memory.Total = (physicalPages * pageSize) / 1048576;

    cores = CPU_Intialize();

    PushSharedMemory->HarwareInformation.Processor.NumberOfCores = cores;
    PushSharedMemory->HarwareInformation.Processor.TjMax = CPU_GetTemperatureMaximal();
    PushSharedMemory->HarwareInformation.Processor.MhzBase = CPU_GetBaseSpeed();

    // Monitors
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


void SetBrightness( int Brightness )
{
    DDCCISetVCPFeature(MonitorHandles[1], VCP_BRIGHTNESS, Brightness);
}


VOID RefreshHardwareInfo()
{
    GPU_INFO gpuInfo;
    MC_TIMING_REPORT timingReport;
    DWORD brightness;

    Memory_Clear(&gpuInfo, sizeof(GPU_INFO));
    PhpUpdateCpuInformation();
    GPU_GetInfo(&gpuInfo);

    PushSharedMemory->HarwareInformation.Processor.MhzCurrent           = CPU_GetSpeed();
    PushSharedMemory->HarwareInformation.Processor.Load                 = GetCpuLoad();
    PushSharedMemory->HarwareInformation.Processor.MaxCoreUsage         = GetMaxCoreLoad();
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

    DDCCIGetVCPFeature(MonitorHandles[1], VCP_BRIGHTNESS, NULL, &brightness, NULL);
    PushSharedMemory->HarwareInformation.Display.Brightness = brightness;

	//Only implimented auto-fan control for Nvidia hardware D:
	if (PushSharedMemory->HarwareInformation.DisplayDevice.VendorId == NVIDIA)
		UpdateFanSpeed();
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
