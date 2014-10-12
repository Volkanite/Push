typedef enum _D3DKMT_QUERYSTATISTICS_TYPE
{
    D3DKMT_QUERYSTATISTICS_ADAPTER                = 0,
    D3DKMT_QUERYSTATISTICS_PROCESS                = 1,
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER        = 2,
    D3DKMT_QUERYSTATISTICS_SEGMENT                = 3,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT        = 4,
    D3DKMT_QUERYSTATISTICS_NODE                   = 5,
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE           = 6,
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE            = 7,
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE    = 8,
} D3DKMT_QUERYSTATISTICS_TYPE;


typedef struct _D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER
{
    UINT32 NbCall;
    UINT32 NbAllocationsReferenced;
    UINT32 MaxNbAllocationsReferenced;
    UINT32 NbNULLReference;
    UINT32 NbWriteReference;
    UINT32 NbRenamedAllocationsReferenced;
    UINT32 NbIterationSearchingRenamedAllocation;
    UINT32 NbLockedAllocationReferenced;
    UINT32 NbAllocationWithValidPrepatchingInfoReferenced;
    UINT32 NbAllocationWithInvalidPrepatchingInfoReferenced;
    UINT32 NbDMABufferSuccessfullyPrePatched;
    UINT32 NbPrimariesReferencesOverflow;
    UINT32 NbAllocationWithNonPreferredResources;
    UINT32 NbAllocationInsertedInMigrationTable;

} D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER;


typedef struct _D3DKMT_QUERYSTATSTICS_RENAMING
{
    UINT32 NbAllocationsRenamed;
    UINT32 NbAllocationsShrinked;
    UINT32 NbRenamedBuffer;
    UINT32 MaxRenamingListLength;
    UINT32 NbFailuresDueToRenamingLimit;
    UINT32 NbFailuresDueToCreateAllocation;
    UINT32 NbFailuresDueToOpenAllocation;
    UINT32 NbFailuresDueToLowResource;
    UINT32 NbFailuresDueToNonRetiredLimit;

} D3DKMT_QUERYSTATSTICS_RENAMING;


typedef struct _D3DKMT_QUERYSTATISTICS_COUNTER
{
    UINT32 Count;
    UINT64 Bytes;

} D3DKMT_QUERYSTATISTICS_COUNTER;


typedef struct _D3DKMT_QUERYSTATSTICS_PREPRATION
{
    UINT32 BroadcastStall;
    UINT32 NbDMAPrepared;
    UINT32 NbDMAPreparedLongPath;
    UINT32 ImmediateHighestPreparationPass;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsTrimmed;
} D3DKMT_QUERYSTATSTICS_PREPRATION;


typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_FAULT
{
    D3DKMT_QUERYSTATISTICS_COUNTER Faults;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsFirstTimeAccess;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsReclaimed;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsMigration;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsIncorrectResource;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsLostContent;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsEvicted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsMEM_RESET;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsUnresetSuccess;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsUnresetFail;
    UINT32 AllocationsUnresetSuccessRead;
    UINT32 AllocationsUnresetFailRead;

    D3DKMT_QUERYSTATISTICS_COUNTER Evictions;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToPreparation;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToLock;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToClose;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToPurge;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToSuspendCPUAccess;
} D3DKMT_QUERYSTATSTICS_PAGING_FAULT;


typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER
{
    UINT64 BytesFilled;
    UINT64 BytesDiscarded;
    UINT64 BytesMappedIntoAperture;
    UINT64 BytesUnmappedFromAperture;
    UINT64 BytesTransferredFromMdlToMemory;
    UINT64 BytesTransferredFromMemoryToMdl;
    UINT64 BytesTransferredFromApertureToMemory;
    UINT64 BytesTransferredFromMemoryToAperture;

} D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER;


typedef struct _D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE
{
    UINT32 NbRangesAcquired;
    UINT32 NbRangesReleased;

} D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE;


typedef struct _D3DKMT_QUERYSTATSTICS_LOCKS
{
    UINT32 NbLocks;
    UINT32 NbLocksWaitFlag;
    UINT32 NbLocksDiscardFlag;
    UINT32 NbLocksNoOverwrite;
    UINT32 NbLocksNoReadSync;
    UINT32 NbLocksLinearization;
    UINT32 NbComplexLocks;

} D3DKMT_QUERYSTATSTICS_LOCKS;


typedef struct _D3DKMT_QUERYSTATSTICS_ALLOCATIONS
{
    D3DKMT_QUERYSTATISTICS_COUNTER Created;
    D3DKMT_QUERYSTATISTICS_COUNTER Destroyed;
    D3DKMT_QUERYSTATISTICS_COUNTER Opened;
    D3DKMT_QUERYSTATISTICS_COUNTER Closed;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedSuccess;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedFail;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedAbandoned;
} D3DKMT_QUERYSTATSTICS_ALLOCATIONS;

typedef struct _D3DKMT_QUERYSTATSTICS_TERMINATIONS
{
    //
    // We separate shared / nonshared because for nonshared we know that every alloc
    // terminated will lead cause a global alloc destroyed, but not for nonshared.
    //
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedNonShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedNonShared;
} D3DKMT_QUERYSTATSTICS_TERMINATIONS;


typedef struct _D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION
{
    UINT32 NbSegments;
    UINT32 NodeCount;
    UINT32 VidPnSourceCount;

    UINT32 VSyncEnabled;
    UINT32 TdrDetectedCount;

    INT64 ZeroLengthDmaBuffers;
    UINT64 RestartedPeriod;

    D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER ReferenceDmaBuffer;
    D3DKMT_QUERYSTATSTICS_RENAMING Renaming;
    D3DKMT_QUERYSTATSTICS_PREPRATION Preparation;
    D3DKMT_QUERYSTATSTICS_PAGING_FAULT PagingFault;
    D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER PagingTransfer;
    D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE SwizzlingRange;
    D3DKMT_QUERYSTATSTICS_LOCKS Locks;
    D3DKMT_QUERYSTATSTICS_ALLOCATIONS Allocations;
    D3DKMT_QUERYSTATSTICS_TERMINATIONS Terminations;

    UINT64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_MEMORY
{
    UINT64 TotalBytesEvicted;
    UINT32 AllocsCommitted;
    UINT32 AllocsResident;
} D3DKMT_QUERYSTATISTICS_MEMORY;


#define D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX 5


typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION
{
    UINT64 CommitLimit;
    UINT64 BytesCommitted;
    UINT64 BytesResident;

    D3DKMT_QUERYSTATISTICS_MEMORY Memory;

    //
    // Boolean, whether this is an aperture segment
    //
    UINT32 Aperture;

    //
    // Breakdown of bytes evicted by priority class
    //
    UINT64 TotalBytesEvictedByPriority[D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX];   //Size = D3DKMT_MaxAllocationPriorityClass

    UINT64 SystemMemoryEndAddress;
    struct
    {
        UINT64 PreservedDuringStandby : 1;
        UINT64 PreservedDuringHibernate : 1;
        UINT64 PartiallyPreservedDuringHibernate : 1;
        UINT64 Reserved : 61;
    } PowerFlags;

    UINT64 Reserved[6];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION;


#define D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT_MAX 16


typedef struct _D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION {
    UINT32 PreemptionCounter[D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT_MAX];
} D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION {
    UINT32  PacketSubmited;
    UINT32  PacketCompleted;
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION;


#define D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_MAX 8

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION {
    UINT32 PacketSubmited;
    UINT32 PacketCompleted;
    UINT32 PacketPreempted;
    UINT32 PacketFaulted;
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION;


#define D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_MAX 4


typedef struct _D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION {
  D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION QueuePacket[D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_MAX];  //Size = D3DKMT_QueuePacketTypeMax
  D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION   DmaPacket[D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_MAX];    //Size = D3DKMT_DmaPacketTypeMax
} D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION {
    LARGE_INTEGER                                 RunningTime;          // Running time in micro-second.
    UINT32                                         ContextSwitch;
    D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION PreemptionStatistics;
    D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION     PacketStatistics;
    UINT64                                        Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_NODE_INFORMATION {
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION GlobalInformation; //Global statistics
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION SystemInformation; //Statistics for system thread
    UINT64                                          Reserved[8];
} D3DKMT_QUERYSTATISTICS_NODE_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION {
    UINT32 Frame;          // both by Blt and Flip.
    UINT32 CancelledFrame; // by restart (flip only).
    UINT32 QueuedPresent;  // queued present.
    UINT64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION {
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION GlobalInformation;   //Global statistics
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION SystemInformation;   //Statistics for system thread
    UINT64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY
{
    UINT64 BytesAllocated;
    UINT64 BytesReserved;
    UINT32 SmallAllocationBlocks;
    UINT32 LargeAllocationBlocks;
    UINT64 WriteCombinedBytesAllocated;
    UINT64 WriteCombinedBytesReserved;
    UINT64 CachedBytesAllocated;
    UINT64 CachedBytesReserved;
    UINT64 SectionBytesAllocated;
    UINT64 SectionBytesReserved;
} D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY;


typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION
{
    UINT32 NodeCount;
    UINT32 VidPnSourceCount;

    D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY SystemMemory;

    UINT64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_DMA_BUFFER
{
    D3DKMT_QUERYSTATISTICS_COUNTER Size;
    UINT32 AllocationListBytes;
    UINT32 PatchLocationListBytes;
} D3DKMT_QUERYSTATISTICS_DMA_BUFFER;


#define D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX 5


typedef struct _D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA
{
    UINT64          TotalBytesEvictedFromProcess;
    UINT64          BytesBySegmentPreference[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
} D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA;


typedef struct _D3DKMT_QUERYSTATISTICS_POLICY
{
    UINT64 PreferApertureForRead[D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX];
    UINT64 PreferAperture[D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS_MAX];
    UINT64 MemResetOnPaging;
    UINT64 RemovePagesFromWorkingSetOnPaging;
    UINT64 MigrationEnabled;
} D3DKMT_QUERYSTATISTICS_POLICY;


typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION
{
    UINT32 NbSegments;
    UINT32 NodeCount;
    UINT32 VidPnSourceCount;

    //
    // Virtual address space used by vidmm for this process
    //
    UINT32 VirtualMemoryUsage;

    D3DKMT_QUERYSTATISTICS_DMA_BUFFER DmaBuffer;
    D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA CommitmentData;
    D3DKMT_QUERYSTATISTICS_POLICY _Policy;

    UINT64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION;


typedef struct _D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY
{
    UINT32 AllocsCommitted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInP[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInNonPreferred;
    UINT64 TotalBytesEvictedDueToPreparation;
} D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY;


typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY
{
    UINT64 UseMRU;
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY;


typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION
{
    UINT64 BytesCommitted;
    UINT64 MaximumWorkingSet;
    UINT64 MinimumWorkingSet;

    UINT32 NbReferencedAllocationEvictedInPeriod;

    D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY VideoMemory;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY _Policy;

    UINT64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION;


typedef enum _D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS
{
    D3DKMT_AllocationPriorityClassMinimum = 0,
    D3DKMT_AllocationPriorityClassLow = 1,
    D3DKMT_AllocationPriorityClassNormal = 2,
    D3DKMT_AllocationPriorityClassHigh = 3,
    D3DKMT_AllocationPriorityClassMaximum = 4,
    D3DKMT_MaxAllocationPriorityClass
} D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS;


typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1
{
    ULONG CommitLimit;
    ULONG BytesCommitted;
    ULONG BytesResident;

    D3DKMT_QUERYSTATISTICS_MEMORY Memory;

    ULONG Aperture; // boolean

    UINT64 TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];

    UINT64 SystemMemoryEndAddress;
    struct
    {
        UINT64 PreservedDuringStandby : 1;
        UINT64 PreservedDuringHibernate : 1;
        UINT64 PartiallyPreservedDuringHibernate : 1;
        UINT64 Reserved : 61;
    } PowerFlags;

    UINT64 Reserved[7];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1;


typedef union _D3DKMT_QUERYSTATISTICS_RESULT
{
    D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION AdapterInformation;
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 SegmentInformationV1; // WIN7
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION SegmentInformation; // WIN8
    D3DKMT_QUERYSTATISTICS_NODE_INFORMATION NodeInformation;
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION VidPnSourceInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION ProcessInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION ProcessAdapterInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION ProcessSegmentInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION ProcessNodeInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION ProcessVidPnSourceInformation;
} D3DKMT_QUERYSTATISTICS_RESULT;


typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT
{
    UINT32 SegmentId; // in: id of node to get statistics for
} D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT;


typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_NODE
{
    UINT32 NodeId;
} D3DKMT_QUERYSTATISTICS_QUERY_NODE;


typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE
{
    UINT32 VidPnSourceId; // in: id of segment to get statistics for
} D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE;


typedef struct _D3DKMT_QUERYSTATISTICS
{
    D3DKMT_QUERYSTATISTICS_TYPE   Type;        // in: type of data requested
    LUID                          AdapterLuid; // in: adapter to get export / statistics from
    VOID                        *hProcess;    // in: process to get statistics for, if required for this query type
    D3DKMT_QUERYSTATISTICS_RESULT QueryResult; // out: requested data

    union
    {
        D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT QuerySegment;                // in: id of segment to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT QueryProcessSegment;         // in: id of segment to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_NODE QueryNode;                      // in: id of node to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_NODE QueryProcessNode;               // in: id of node to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryVidPnSource;        // in: id of vidpnsource to get statistics for
        D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryProcessVidPnSource; // in: id of vidpnsource to get statistics for
    };
} D3DKMT_QUERYSTATISTICS;


typedef UINT32 D3DKMT_HANDLE;


typedef struct _D3DKMT_OPENADAPTERFROMDEVICENAME
{
    const WCHAR                     *pDeviceName;    // in:  NULL terminated string containing the device name to open
    D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
    LUID                            AdapterLuid;    // out: adapter LUID
} D3DKMT_OPENADAPTERFROMDEVICENAME;


typedef INT32 (__stdcall *TYPE_D3DKMTOpenAdapterFromDeviceName)(D3DKMT_OPENADAPTERFROMDEVICENAME *);
typedef INT32 (__stdcall *TYPE_D3DKMTQueryStatistics)(const D3DKMT_QUERYSTATISTICS *);