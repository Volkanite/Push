#include <sltypes.h>
#include <slnt.h>
#include <slntuapi.h>
#include <slmodule.h>
#include <sld3dkmt.h>
#include <string.h>

#include "push.h"


GUID GUID_DISPLAY_DEVICE_ARRIVAL_I  = { 0x1ca05180, 0xa699, 0x450a, { 0x9a, 0x0c, 0xde, 0x4f, 0xbe, 0x3d, 0xdd, 0x89 } };


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
    _In_ UINT32 NumberOfSegments
    );


UINT32 EtGpuTotalNodeCount;
PETP_GPU_ADAPTER D3dkmt_GpuAdapter;
UINT32 EtGpuTotalNodeCount;
UINT32 EtGpuTotalSegmentCount;
UINT32 EtGpuNextNodeIndex = 0;
UINT32 *EtGpuNodeBitMapBuffer;


#define BYTES_NEEDED_FOR_BITS(Bits) ((((Bits) + sizeof(UINT32) * 8 - 1) / 8) & ~(UINT32)(sizeof(UINT32) - 1)) // divide round up
#define DIGCF_PRESENT           0x00000002
#define DIGCF_DEVICEINTERFACE   0x00000010
#define FIELD_OFFSET(type, field)    ((INT32)(INT32)&(((type *)0)->field))

#define PhUpdateDelta(DltMgr, NewValue) \
    ((DltMgr)->Delta = (NewValue) - (DltMgr)->Value, \
    (DltMgr)->Value = (NewValue), (DltMgr)->Delta)


VOID __stdcall RtlInitializeBitMap(
  _Out_  RTL_BITMAP* BitMapHeader,
  _In_   UINT32 *BitMapBuffer,
  _In_   UINT32 SizeOfBitMap
);
VOID
__stdcall
RtlSetBits(
    RTL_BITMAP* BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToSet
    );
RTL_BITMAP EtGpuNodeBitMap;


LARGE_INTEGER EtClockTotalRunningTimeFrequency;
PH_UINT64_DELTA EtClockTotalRunningTimeDelta;
PH_UINT64_DELTA EtGpuTotalRunningTimeDelta;
PH_UINT64_DELTA EtGpuSystemRunningTimeDelta;

FLOAT EtGpuNodeUsage;
UINT64 EtGpuDedicatedLimit;


BOOLEAN
RtlCheckBit(
    RTL_BITMAP* BitMapHeader,
    ULONG BitPosition
    )
{
    return (((LONG*)BitMapHeader->Buffer)[BitPosition / 32] >> (BitPosition % 32)) & 0x1;
}

PETP_GPU_ADAPTER
AllocateGpuAdapter(UINT32 NumberOfSegments)
{
    PETP_GPU_ADAPTER adapter;
    UINT32 sizeNeeded;

    sizeNeeded = FIELD_OFFSET(ETP_GPU_ADAPTER, ApertureBitMapBuffer);
    sizeNeeded += BYTES_NEEDED_FOR_BITS(NumberOfSegments);

    //adapter = SlAllocate(sizeNeeded);
    adapter = RtlAllocateHeap(
                PushHeapHandle,
                0,
                sizeNeeded
                );

    memset(adapter, 0, sizeNeeded);

    return adapter;
}


UINT8
D3DKMTGetGpuUsage()
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


UINT64
D3DKMTGetMemoryUsage()
{
    ULONG i;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    UINT64 dedicatedUsage;

    dedicatedUsage = 0;

    for (i = 0; i < D3dkmt_GpuAdapter->SegmentCount; i++)
    {
        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));

        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
        queryStatistics.AdapterLuid = D3dkmt_GpuAdapter->AdapterLuid;
        queryStatistics.QuerySegment.SegmentId = i;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            UINT64 bytesCommitted;

            bytesCommitted = queryStatistics.QueryResult.SegmentInformationV1.BytesCommitted;

            if (!RtlCheckBit(&D3dkmt_GpuAdapter->ApertureBitMap, i))
                dedicatedUsage += bytesCommitted;
        }
    }

    return dedicatedUsage;
}


VOID
D3DKMTInitialize()
{
    VOID *gdi32 = NULL;
    VOID *deviceInfoSet;
    UINT32 result;
    UINT32 memberIndex;
    UINT32 detailDataSize;
    SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W   *detailData;
    SP_DEVINFO_DATA                     deviceInfoData;
    D3DKMT_OPENADAPTERFROMDEVICENAME    openAdapterFromDeviceName;
    D3DKMT_QUERYSTATISTICS              queryStatistics;

    //gdi32 = PushLoadLibrary(L"gdi32.dll");
    gdi32 = SlLoadLibrary(L"gdi32.dll");

    if (!gdi32)
    {
        return;
    }

    D3DKMTOpenAdapterFromDeviceName = (TYPE_D3DKMTOpenAdapterFromDeviceName) GetProcAddress(
                                                                                gdi32,
                                                                                "D3DKMTOpenAdapterFromDeviceName"
                                                                                );

    D3DKMTQueryStatistics = (TYPE_D3DKMTQueryStatistics) GetProcAddress(gdi32, "D3DKMTQueryStatistics");

    if (!D3DKMTOpenAdapterFromDeviceName || !D3DKMTQueryStatistics)
    {
        return;
    }

    deviceInfoSet = SetupDiGetClassDevsW(&GUID_DISPLAY_DEVICE_ARRIVAL_I,
                                         NULL, NULL,
                                         DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (!deviceInfoSet)
    {
        return;
    }

    memberIndex = 0;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &GUID_DISPLAY_DEVICE_ARRIVAL_I, memberIndex, &deviceInterfaceData))
    {
        detailDataSize = 0x100;
        //detailData = SlAllocate(detailDataSize);
        detailData = RtlAllocateHeap(PushHeapHandle, 0, detailDataSize);
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
            openAdapterFromDeviceName.pDeviceName = detailData->DevicePath;

            if (NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName)))
            {
                memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));

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
                        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));

                        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
                        queryStatistics.AdapterLuid = D3dkmt_GpuAdapter->AdapterLuid;
                        queryStatistics.QuerySegment.SegmentId = i;

                        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
                        {
                            UINT64 commitLimit;
                            UINT32 aperature;

                            commitLimit = queryStatistics.QueryResult.SegmentInformationV1.CommitLimit;
                            aperature = queryStatistics.QueryResult.SegmentInformationV1.Aperture;

                            if (aperature)
                                RtlSetBits(&D3dkmt_GpuAdapter->ApertureBitMap, i, 1);
                            else
                                EtGpuDedicatedLimit += commitLimit;
                        }
                    }
                }
            }
        }

        //SlFree(detailData);
        RtlFreeHeap(PushHeapHandle, 0, detailData);

        memberIndex++;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    //EtGpuNodeBitMapBuffer = SlAllocate(BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount));
    EtGpuNodeBitMapBuffer = RtlAllocateHeap(PushHeapHandle, 0, BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount));
    RtlInitializeBitMap(&EtGpuNodeBitMap, EtGpuNodeBitMapBuffer, EtGpuTotalNodeCount);
    //EtGpuNodeBitMapBitsSet = 1;

    //EtGpuNodesTotalRunningTimeDelta = SlAllocate(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
    EtGpuNodesTotalRunningTimeDelta = RtlAllocateHeap(PushHeapHandle, 0, sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);

    memset(EtGpuNodesTotalRunningTimeDelta, 0, sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
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
        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));

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