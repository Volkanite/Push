#include <sl.h>
#include <sld3dkmt.h>
#include <push.h>
#include <hardware.h>
#include "gpu.h"


typedef struct _PH_UINT64_DELTA
{
    UINT64 Value;
    UINT64 Delta;
} PH_UINT64_DELTA, *PPH_UINT64_DELTA;


TYPE_D3DKMTOpenAdapterFromDeviceName    D3DKMTOpenAdapterFromDeviceName = NULL;
TYPE_D3DKMTQueryStatistics              D3DKMTQueryStatistics           = NULL;
PPH_UINT64_DELTA EtGpuNodesTotalRunningTimeDelta;
struct _PH_STRING;
typedef struct _PH_STRING *PPH_STRING;
typedef struct _ETP_GPU_ADAPTER
{
    LUID AdapterLuid;
    PPH_STRING Description;
    ULONG SegmentCount;
    ULONG NodeCount;
    ULONG FirstNodeIndex;

    RTL_BITMAP ApertureBitMap;
    ULONG ApertureBitMapBuffer[1];
} ETP_GPU_ADAPTER, *PETP_GPU_ADAPTER;


VOID InitializeD3DStatistics();
VOID UpdateNodeInformation();
PETP_GPU_ADAPTER AllocateGpuAdapter(
    UINT32 NumberOfSegments
    );


UINT32 EtGpuTotalNodeCount;
PETP_GPU_ADAPTER D3dkmt_GpuAdapter;
UINT32 EtGpuTotalSegmentCount;
UINT32 EtGpuNextNodeIndex = 0;
UINT32 *EtGpuNodeBitMapBuffer;
BOOLEAN D3DKMT_Initialized;


#define BYTES_NEEDED_FOR_BITS(Bits) ((((Bits) + sizeof(UINT32) * 8 - 1) / 8) & ~(UINT32)(sizeof(UINT32) - 1)) // divide round up


#define FIELD_OFFSET(type, field)    ((INT32)(INT32)&(((type *)0)->field))

#define PhUpdateDelta(DltMgr, NewValue) \
    ((DltMgr)->Delta = (NewValue) - (DltMgr)->Value, \
    (DltMgr)->Value = (NewValue), (DltMgr)->Delta)

typedef struct _OSVERSIONINFOW {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    WCHAR  szCSDVersion[128];     // Maintenance string for PSS usage
} OSVERSIONINFOW, *POSVERSIONINFOW, *LPOSVERSIONINFOW, RTL_OSVERSIONINFOW, *PRTL_OSVERSIONINFOW;

VOID __stdcall RtlInitializeBitMap(
  RTL_BITMAP* BitMapHeader,
  UINT32 *BitMapBuffer,
  UINT32 SizeOfBitMap
);
VOID
__stdcall
RtlSetBits(
    RTL_BITMAP* BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToSet
    );

NTSTATUS __stdcall RtlGetVersion(
    RTL_OSVERSIONINFOW* lpVersionInformation
    );

RTL_BITMAP EtGpuNodeBitMap;
LARGE_INTEGER EtClockTotalRunningTimeFrequency;
PH_UINT64_DELTA EtClockTotalRunningTimeDelta;
PH_UINT64_DELTA EtGpuTotalRunningTimeDelta;
PH_UINT64_DELTA EtGpuSystemRunningTimeDelta;
FLOAT EtGpuNodeUsage;
UINT64 EtGpuDedicatedLimit;
ULONG WindowsVersion;
#define WINDOWS_8 62


BOOLEAN
RtlCheckBit(
    RTL_BITMAP* BitMapHeader,
    ULONG BitPosition
    )
{
    return (((LONG*)BitMapHeader->Buffer)[BitPosition / 32] >> (BitPosition % 32)) & 0x1;
}


PETP_GPU_ADAPTER AllocateGpuAdapter( UINT32 NumberOfSegments )
{
    PETP_GPU_ADAPTER adapter;
    UINT32 sizeNeeded;

    sizeNeeded = FIELD_OFFSET(ETP_GPU_ADAPTER, ApertureBitMapBuffer);
    sizeNeeded += BYTES_NEEDED_FOR_BITS(NumberOfSegments);

    adapter = (PETP_GPU_ADAPTER) Memory_Allocate(sizeNeeded);

    Memory_Clear(adapter, sizeNeeded);

    return adapter;
}


UINT8 D3DKMT_GetGpuUsage()
{
    double elapsedTime; // total GPU node elapsed time in micro-seconds
    INT32 usage = 0;


    UpdateNodeInformation();

    elapsedTime = (double)EtClockTotalRunningTimeDelta.Delta * 10000000 / EtClockTotalRunningTimeFrequency.QuadPart;

    if (elapsedTime != 0)
    {
        EtGpuNodeUsage = (float) (EtGpuTotalRunningTimeDelta.Delta / elapsedTime);
        usage = EtGpuNodeUsage * 100;
    }
    else
        EtGpuNodeUsage = 0;

    if (EtGpuNodeUsage > 1)
        EtGpuNodeUsage = 1;

     // Clip calculated usage to [0-100] range to filter calculation non-ideality.

     if (usage < 0)
         usage = 0;

     if (usage > 100)
         usage = 100;

    return usage;
}


UINT64 D3DKMTGetMemoryUsage()
{
    ULONG i;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    UINT64 dedicatedUsage;

    dedicatedUsage = 0;

    for (i = 0; i < D3dkmt_GpuAdapter->SegmentCount; i++)
    {
        Memory_Clear(&queryStatistics, sizeof(D3DKMT_QUERYSTATISTICS));

        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
        queryStatistics.AdapterLuid = D3dkmt_GpuAdapter->AdapterLuid;
        queryStatistics.QuerySegment.SegmentId = i;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            UINT64 bytesCommitted;

            if (WindowsVersion == WINDOWS_8)
            {
                bytesCommitted = queryStatistics.QueryResult.SegmentInformation.BytesResident;
            }
            else
            {
                bytesCommitted = queryStatistics.QueryResult.SegmentInformationV1.BytesResident;
            }

            if (!RtlCheckBit(&D3dkmt_GpuAdapter->ApertureBitMap, i))
                dedicatedUsage += bytesCommitted;
        }
    }

    return dedicatedUsage;
}


D3DKMT_OPENADAPTERFROMDEVICENAME    openAdapterFromDeviceName;


VOID D3DKMTInitialize()
{
    HANDLE gdi32 = NULL;
    D3DKMT_QUERYSTATISTICS              queryStatistics;
    RTL_OSVERSIONINFOW versionInfo;

    if (D3DKMT_Initialized)
    {
        return;
    }

    RtlGetVersion(&versionInfo);

    if (versionInfo.dwMajorVersion == 6 && versionInfo.dwMinorVersion == 2)
    {
        WindowsVersion = WINDOWS_8;
    }

    gdi32 = Module_Load(L"gdi32.dll");

    if (!gdi32)
    {
        return;
    }

    D3DKMTOpenAdapterFromDeviceName = (TYPE_D3DKMTOpenAdapterFromDeviceName) Module_GetProcedureAddress(
                                                                                gdi32,
                                                                                "D3DKMTOpenAdapterFromDeviceName"
                                                                                );

    D3DKMTQueryStatistics = (TYPE_D3DKMTQueryStatistics) Module_GetProcedureAddress(gdi32, "D3DKMTQueryStatistics");

    if (!D3DKMTOpenAdapterFromDeviceName || !D3DKMTQueryStatistics)
    {
        return;
    }

    wchar_t devicePath[260];

    GetDisplayAdapterDevicePath(devicePath);

    openAdapterFromDeviceName.pDeviceName = devicePath;

            if (NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName)))
            {
                Memory_Clear(&queryStatistics, sizeof(D3DKMT_QUERYSTATISTICS));

                queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
                queryStatistics.AdapterLuid = openAdapterFromDeviceName.AdapterLuid;

                if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
                {
                    UINT32 i;

                    D3dkmt_GpuAdapter = AllocateGpuAdapter(queryStatistics.QueryResult.AdapterInformation.NbSegments);

                    D3dkmt_GpuAdapter->AdapterLuid       = openAdapterFromDeviceName.AdapterLuid;
                    D3dkmt_GpuAdapter->NodeCount     = queryStatistics.QueryResult.AdapterInformation.NodeCount;
                    D3dkmt_GpuAdapter->SegmentCount  = queryStatistics.QueryResult.AdapterInformation.NbSegments;

                    RtlInitializeBitMap(
                        &D3dkmt_GpuAdapter->ApertureBitMap,
                        D3dkmt_GpuAdapter->ApertureBitMapBuffer,
                        queryStatistics.QueryResult.AdapterInformation.NbSegments
                        );

                    EtGpuTotalNodeCount += D3dkmt_GpuAdapter->NodeCount;

                    EtGpuTotalSegmentCount += D3dkmt_GpuAdapter->SegmentCount;

                    D3dkmt_GpuAdapter->FirstNodeIndex = EtGpuNextNodeIndex;
                    EtGpuNextNodeIndex += D3dkmt_GpuAdapter->NodeCount;

                    for (i = 0; i < D3dkmt_GpuAdapter->SegmentCount; i++)
                    {
                        Memory_Clear(&queryStatistics, sizeof(D3DKMT_QUERYSTATISTICS));

                        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
                        queryStatistics.AdapterLuid = D3dkmt_GpuAdapter->AdapterLuid;
                        queryStatistics.QuerySegment.SegmentId = i;

                        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
                        {
                            UINT64 commitLimit;
                            UINT32 aperature;

                            if (WindowsVersion == WINDOWS_8)
                            {
                                commitLimit = queryStatistics.QueryResult.SegmentInformation.CommitLimit;
                                aperature = queryStatistics.QueryResult.SegmentInformation.Aperture;
                            }
                            else
                            {
                                commitLimit = queryStatistics.QueryResult.SegmentInformationV1.CommitLimit;
                                aperature = queryStatistics.QueryResult.SegmentInformationV1.Aperture;
                            }

                            if (aperature)
                                RtlSetBits(&D3dkmt_GpuAdapter->ApertureBitMap, i, 1);
                            else
                                EtGpuDedicatedLimit += commitLimit;
                        }
                    }
                }
            }

    EtGpuNodeBitMapBuffer = (UINT32*) Memory_Allocate(BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount));
    
    RtlInitializeBitMap(&EtGpuNodeBitMap, EtGpuNodeBitMapBuffer, EtGpuTotalNodeCount);

    EtGpuNodesTotalRunningTimeDelta = (PPH_UINT64_DELTA) Memory_Allocate(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);

    Memory_Clear(EtGpuNodesTotalRunningTimeDelta, sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);

    D3DKMT_Initialized = TRUE;
}


VOID
UpdateNodeInformation()
{
    UINT32 j;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    UINT64 totalRunningTime;
    UINT64 systemRunningTime;
    LARGE_INTEGER performanceCounter;
    static BOOLEAN initialized = FALSE;

    //check if initialized
    if (!initialized)
    {
        D3DKMTInitialize();
        initialized = TRUE;
    }

    totalRunningTime = 0;
    systemRunningTime = 0;

    if (D3dkmt_GpuAdapter == NULL)
    {
        return;
    }

    for (j = 0; j < D3dkmt_GpuAdapter->NodeCount; j++)
    {
        Memory_Clear(&queryStatistics, sizeof(D3DKMT_QUERYSTATISTICS));

        queryStatistics.Type                = D3DKMT_QUERYSTATISTICS_NODE;
        queryStatistics.AdapterLuid         = D3dkmt_GpuAdapter->AdapterLuid;
        queryStatistics.QueryNode.NodeId    = j;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            UINT32 nodeIndex;

            nodeIndex = D3dkmt_GpuAdapter->FirstNodeIndex + j;

            PhUpdateDelta(
                &EtGpuNodesTotalRunningTimeDelta[nodeIndex],
                queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart
                );

            totalRunningTime += queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
            systemRunningTime += queryStatistics.QueryResult.NodeInformation.SystemInformation.RunningTime.QuadPart;
        }
    }

    NtQueryPerformanceCounter(&performanceCounter, &EtClockTotalRunningTimeFrequency);

    PhUpdateDelta(&EtClockTotalRunningTimeDelta, performanceCounter.QuadPart);
    PhUpdateDelta(&EtGpuTotalRunningTimeDelta, totalRunningTime);
    PhUpdateDelta(&EtGpuSystemRunningTimeDelta, systemRunningTime);
}


typedef enum _D3DKMT_ESCAPETYPE
{
    D3DKMT_ESCAPE_DRIVERPRIVATE = 0,
    D3DKMT_ESCAPE_VIDMM = 1,
    D3DKMT_ESCAPE_TDRDBGCTRL = 2,
    D3DKMT_ESCAPE_VIDSCH = 3,
    D3DKMT_ESCAPE_DEVICE = 4,
    D3DKMT_ESCAPE_DMM = 5,
    D3DKMT_ESCAPE_DEBUG_SNAPSHOT = 6,
    D3DKMT_ESCAPE_SETDRIVERUPDATESTATUS = 7,
    D3DKMT_ESCAPE_DRT_TEST = 8,
    D3DKMT_ESCAPE_DIAGNOSTICS = 9
} D3DKMT_ESCAPETYPE;

typedef struct _D3DDDI_ESCAPEFLAGS
{
    union
    {
        struct
        {
            UINT32    HardwareAccess : 1;    // 0x00000001
            UINT32    Reserved : 31;    // 0xFFFFFFFE
        };
        UINT32        Value;
    };
} D3DDDI_ESCAPEFLAGS;

typedef struct _D3DKMT_ESCAPE
{
    D3DKMT_HANDLE       hAdapter;               // in: adapter handle
    D3DKMT_HANDLE       hDevice;                // in: device handle [Optional]
    D3DKMT_ESCAPETYPE   Type;                   // in: escape type.
    D3DDDI_ESCAPEFLAGS  Flags;                  // in: flags
    VOID*               pPrivateDriverData;     // in/out: escape data
    UINT32                PrivateDriverDataSize;  // in: size of escape data
    D3DKMT_HANDLE       hContext;               // in: context handle [Optional]
} D3DKMT_ESCAPE;

typedef NTSTATUS (__stdcall *TYPE_D3DKMTEscape)(
    const D3DKMT_ESCAPE *pData
    );

TYPE_D3DKMTEscape D3DKMTEscape;

VOID D3DKMT_GetPrivateDriverData( VOID* PrivateDriverData, UINT32 PrivateDriverDataSize )
{
    D3DKMT_ESCAPE driverInformation = { 0 };

    driverInformation.hAdapter = openAdapterFromDeviceName.hAdapter;
    driverInformation.hDevice = NULL;
    driverInformation.Type = D3DKMT_ESCAPE_DRIVERPRIVATE;
    driverInformation.Flags.Value = 0;
    driverInformation.pPrivateDriverData = PrivateDriverData;
    driverInformation.PrivateDriverDataSize = PrivateDriverDataSize;
    driverInformation.hContext = NULL;

    D3DKMTEscape = (TYPE_D3DKMTEscape) Module_GetProcedureAddress(Module_Load(L"gdi32.dll"), "D3DKMTEscape");
    D3DKMTEscape(&driverInformation);
}


UINT16 D3DKMT_GetEngineClock(){ return 0; };
UINT16 D3DKMT_GetMemoryClock(){ return 0; };
UINT16 D3DKMT_GetMaxEngineClock(){ return 0; };
UINT16 D3DKMT_GetMaxMemoryClock(){ return 0; };
UINT64 D3DKMT_GetTotalMemory(){ return 0; };
UINT64 D3DKMT_GetFreeMemory(){ return 0; };
UINT8  D3DKMT_GetTemperature(){ return 0; };
VOID   D3DKMT_ForceMaximumClocks(){};
