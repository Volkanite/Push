#include <sl.h>
#include <string.h>
#include <push.h>


UINT64 DiskBytesDelta;
UINT32 DiskResponseTime;


typedef struct _EVENT_TRACE_HEADER {        // overlays WNODE_HEADER
    UINT16          Size;                   // Size of entire record
    union {
        UINT16      FieldTypeFlags;         // Indicates valid fields
        struct {
            UCHAR   HeaderType;             // Header type - internal use only
            UCHAR   MarkerFlags;            // Marker - internal use only
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union {
        ULONG       Version;
        struct {
            UCHAR   Type;                   // event type
            UCHAR   Level;                  // trace instrumentation level
            UINT16  Version;                // version of trace record
        } Class;
    };
    ULONG           ThreadId;               // Thread Id
    ULONG           ProcessId;              // Process Id
    LARGE_INTEGER   TimeStamp;              // time when event happens
    union {
        GUID        Guid;                   // Guid that identifies event
        UINT64   GuidPtr;                // use with WNODE_FLAG_USE_GUID_PTR
    } DUMMYUNIONNAME3;
    union {
        struct {
            ULONG   KernelTime;             // Kernel Mode CPU ticks
            ULONG   UserTime;               // User mode CPU ticks
        } DUMMYSTRUCTNAME;
        UINT64     ProcessorTime;          // Processor Clock
        struct {
            ULONG   ClientContext;          // Reserved
            ULONG   Flags;                  // Event Flags
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME4;
} EVENT_TRACE_HEADER, *PEVENT_TRACE_HEADER;


typedef struct _ETW_BUFFER_CONTEXT {
    UCHAR   ProcessorNumber;
    UCHAR   Alignment;
    UINT16  LoggerId;
} ETW_BUFFER_CONTEXT, *PETW_BUFFER_CONTEXT;


typedef struct _EVENT_TRACE {
    EVENT_TRACE_HEADER      Header;             // Event trace header
    ULONG                   InstanceId;         // Instance Id of this event
    ULONG                   ParentInstanceId;   // Parent Instance Id.
    GUID                    ParentGuid;         // Parent Guid;
    VOID*                   MofData;            // Pointer to Variable Data
    ULONG                   MofLength;          // Variable Datablock Length
    union {
        ULONG               ClientContext;
        ETW_BUFFER_CONTEXT  BufferContext;
    } DUMMYUNIONNAME;
} EVENT_TRACE, *PEVENT_TRACE;


typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;


typedef struct _TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;


typedef struct _TRACE_LOGFILE_HEADER {
    ULONG           BufferSize;         // Logger buffer size in Kbytes
    union {
        ULONG       Version;            // Logger version
        struct {
            UCHAR   MajorVersion;
            UCHAR   MinorVersion;
            UCHAR   SubVersion;
            UCHAR   SubMinorVersion;
        } VersionDetail;
    } DUMMYUNIONNAME;
    ULONG           ProviderVersion;    // defaults to NT version
    ULONG           NumberOfProcessors; // Number of Processors
    LARGE_INTEGER   EndTime;            // Time when logger stops
    ULONG           TimerResolution;    // assumes timer is constant!!!
    ULONG           MaximumFileSize;    // Maximum in Mbytes
    ULONG           LogFileMode;        // specify logfile mode
    ULONG           BuffersWritten;     // used to file start of Circular File
    union {
        GUID LogInstanceGuid;           // For RealTime Buffer Delivery
        struct {
            ULONG   StartBuffers;       // Count of buffers written at start.
            ULONG   PointerSize;        // Size of pointer type in bits
            ULONG   EventsLost;         // Events losts during log session
            ULONG   CpuSpeedInMHz;      // Cpu Speed in MHz
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME2;
#if defined(_WMIKM_)
    PWCHAR          LoggerName;
    PWCHAR          LogFileName;
    RTL_TIME_ZONE_INFORMATION TimeZone;
#else
    WCHAR*          LoggerName;
    WCHAR*          LogFileName;
    TIME_ZONE_INFORMATION TimeZone;
#endif
    LARGE_INTEGER   BootTime;
    LARGE_INTEGER   PerfFreq;           // Reserved
    LARGE_INTEGER   StartTime;          // Reserved
    ULONG           ReservedFlags;      // ClockType
    ULONG           BuffersLost;
} TRACE_LOGFILE_HEADER, *PTRACE_LOGFILE_HEADER;


typedef struct _EVENT_RECORD
                EVENT_RECORD, *PEVENT_RECORD;

typedef struct _EVENT_TRACE_LOGFILEW
                EVENT_TRACE_LOGFILEW, *PEVENT_TRACE_LOGFILEW;

typedef ULONG (__stdcall * PEVENT_TRACE_BUFFER_CALLBACKW)
                (PEVENT_TRACE_LOGFILEW Logfile);

typedef VOID (__stdcall *PEVENT_CALLBACK)( PEVENT_TRACE pEvent );

typedef VOID (__stdcall *PEVENT_RECORD_CALLBACK) (PEVENT_RECORD EventRecord);


typedef struct _EVENT_TRACE_LOGFILEW {
    WCHAR*                  LogFileName;      // Logfile Name
    WCHAR*                  LoggerName;       // LoggerName
    INT64                CurrentTime;      // timestamp of last event
    ULONG                   BuffersRead;      // buffers read to date
    union {
        // Mode of the logfile
        ULONG               LogFileMode;
        // Processing flags used on Vista and above
        ULONG               ProcessTraceMode;
    };
    EVENT_TRACE             CurrentEvent;     // Current Event from this stream.
    TRACE_LOGFILE_HEADER    LogfileHeader;    // logfile header structure
    PEVENT_TRACE_BUFFER_CALLBACKW             // callback before each buffer
                            BufferCallback;   // is read
    //
    // following variables are filled for BufferCallback.
    //
    ULONG                   BufferSize;
    ULONG                   Filled;
    ULONG                   EventsLost;
    //
    // following needs to be propaged to each buffer
    //
    union {
        // Callback with EVENT_TRACE
        PEVENT_CALLBACK         EventCallback;
        // Callback with EVENT_RECORD on Vista and above
        PEVENT_RECORD_CALLBACK  EventRecordCallback;
    } DUMMYUNIONNAME2;

    ULONG                   IsKernelTrace;    // TRUE for kernel logfile

    VOID*                   Context;          // reserved for internal use

}EVENT_TRACE_LOGFILEW;


typedef struct
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONG TransferSize;
    ULONG ResponseTime;
    UINT64 ByteOffset;
    UINT64 FileObject;
    UINT64 Irp;
    UINT64 HighResResponseTime;
    ULONG IssuingThreadId;
} DiskIo_TypeGroup1;


typedef struct _WNODE_HEADER
{
    ULONG BufferSize;        // Size of entire buffer inclusive of this ULONG
    ULONG ProviderId;    // Provider Id of driver returning this buffer
    union
    {
        UINT64 HistoricalContext;  // Logger use
        struct
            {
            ULONG Version;           // Reserved
            ULONG Linkage;           // Linkage field reserved for WMI
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    union
    {
        ULONG CountLost;         // Reserved
        VOID* KernelHandle;     // Kernel handle for data block
        LARGE_INTEGER TimeStamp; // Timestamp as returned in units of 100ns
                                 // since 1/1/1601
    } DUMMYUNIONNAME2;
    GUID Guid;                  // Guid for data block returned with results
    ULONG ClientContext;
    ULONG Flags;             // Flags, see below
} WNODE_HEADER, *PWNODE_HEADER;


typedef struct _EVENT_TRACE_PROPERTIES {
    WNODE_HEADER Wnode;
//
// data provided by caller
    ULONG BufferSize;                   // buffer size for logging (kbytes)
    ULONG MinimumBuffers;               // minimum to preallocate
    ULONG MaximumBuffers;               // maximum buffers allowed
    ULONG MaximumFileSize;              // maximum logfile size (in MBytes)
    ULONG LogFileMode;                  // sequential, circular
    ULONG FlushTimer;                   // buffer flush timer, in seconds
    ULONG EnableFlags;                  // trace enable flags
    LONG  AgeLimit;                     // unused

// data returned to caller
    ULONG NumberOfBuffers;              // no of buffers in use
    ULONG FreeBuffers;                  // no of buffers free
    ULONG EventsLost;                   // event records lost
    ULONG BuffersWritten;               // no of buffers written to file
    ULONG LogBuffersLost;               // no of logfile write failures
    ULONG RealTimeBuffersLost;          // no of rt delivery failures
    VOID* LoggerThreadId;              // thread id of Logger
    ULONG LogFileNameOffset;            // Offset to LogFileName
    ULONG LoggerNameOffset;             // Offset to LoggerName
} EVENT_TRACE_PROPERTIES;


#define EVENT_TRACE_REAL_TIME_MODE 0x00000100
#define KERNEL_LOGGER_NAME L"NT Kernel Logger"
#define EVENT_TRACE_FLAG_DISK_IO 0x00000100
#define WNODE_FLAG_TRACED_GUID   0x00020000
#define EVENT_TRACE_FILE_MODE_SEQUENTIAL 0x00000001
#define EVENT_TRACE_TYPE_IO_READ 0x0A
#define EVENT_TRACE_TYPE_IO_WRITE 0x0B

static GUID SystemTraceControlGuid_I = { 0x9e814aad, 0x3204, 0x11d2, { 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39 } };
static GUID DiskIoGuid_I = { 0x3d6fa8d4, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };

extern "C"
{
ULONG __stdcall StartTraceW(
    UINT64* TraceHandle,
    WCHAR* InstanceName,
    EVENT_TRACE_PROPERTIES* Properties
    );

UINT64 __stdcall OpenTraceW(
    EVENT_TRACE_LOGFILEW* Logfile
    );

ULONG __stdcall SetTraceCallback(
    const GUID* pGuid,
    PEVENT_CALLBACK EventCallback
    );

ULONG __stdcall ProcessTrace(
    UINT64* HandleArray,
    ULONG HandleCount,
    FILETIME* StartTime,
    FILETIME* EndTime
    );
}


typedef struct _CIRCULAR_BUFFER
{
    ULONG Size;
    ULONG Count;
    LONG Index;
    UINT32* Data;
} CIRCULAR_BUFFER;


VOID InitializeCircularBuffer( CIRCULAR_BUFFER* Buffer, ULONG Size )
{
    Buffer->Size = Size;
    Buffer->Count = 0;
    Buffer->Index = 0;

	Buffer->Data = (UINT32*) Memory::Allocate(sizeof(ULONG) * Buffer->Size);
}


VOID
AddItemCircularBuffer( CIRCULAR_BUFFER* Buffer, ULONG Value )
{
    ULONG size;

    size = Buffer->Size;
    Buffer->Data[Buffer->Index = (((Buffer->Index - 1) % size) + size) % size] = Value;

    if (Buffer->Count < Buffer->Size)
        Buffer->Count++;
}


UINT32 GetItemCircularBuffer( CIRCULAR_BUFFER* Buffer, LONG Index )
{
    ULONG size;

    size = Buffer->Size;
    // Modulo is dividend-based.
    return Buffer->Data[(((Buffer->Index + Index) % size) + size) % size];
}


UINT32
GetAverageCircularBuffer(
    CIRCULAR_BUFFER* Buffer
    )
{
    UINT32 i;
    UINT64 total = 0;

    for (i = 0; i < Buffer->Count; i++)
        total += GetItemCircularBuffer(Buffer, i);

    return total / Buffer->Count;
}


CIRCULAR_BUFFER DiskReadWriteHistory;


VOID __stdcall DiskEvents( EVENT_TRACE* EventTrace )
{
    DiskIo_TypeGroup1 *data = (DiskIo_TypeGroup1*) EventTrace->MofData;

    if (EventTrace->Header.Class.Type == EVENT_TRACE_TYPE_IO_READ
        || EventTrace->Header.Class.Type == EVENT_TRACE_TYPE_IO_WRITE)
    {
        DiskBytesDelta += data->TransferSize;
        DiskResponseTime = data->HighResResponseTime;
    }
}


static
DWORD
__stdcall
MonitorThread(VOID *h)
{
    EVENT_TRACE_PROPERTIES *traceProperties;
    UINT32 bufferSize;
    EVENT_TRACE_LOGFILEW trace = { 0 };
    UINT64 traceHandle = NULL, sessionHandle = NULL;

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAME);
    traceProperties = (EVENT_TRACE_PROPERTIES*) Memory::Allocate(bufferSize);

    memset(traceProperties, 0, sizeof(EVENT_TRACE_PROPERTIES));

    traceProperties->Wnode.BufferSize = bufferSize;
    traceProperties->Wnode.Guid = SystemTraceControlGuid_I;
    traceProperties->Wnode.ClientContext = 1;
    traceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    traceProperties->MinimumBuffers = 1;
    traceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    traceProperties->FlushTimer = 1;
    traceProperties->EnableFlags = EVENT_TRACE_FLAG_DISK_IO;
    traceProperties->LogFileNameOffset = 0;
    traceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    StartTraceW(&sessionHandle, KERNEL_LOGGER_NAME, traceProperties);

    trace.LoggerName = KERNEL_LOGGER_NAME;
    trace.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

    traceHandle = OpenTraceW(&trace);

    SetTraceCallback(&DiskIoGuid_I, DiskEvents);
    ProcessTrace(&traceHandle, 1, 0, 0);

    return 0;
}


VOID
DiskStartMonitoring()
{
    InitializeCircularBuffer(&DiskReadWriteHistory, 2);
    //CreateThread(0,0, &MonitorThread, 0,0,0);
    CreateRemoteThread(NtCurrentProcess(), 0, 0, &MonitorThread, 0, 0, 0);
}


UINT64
DiskGetBytesDelta()
{
    AddItemCircularBuffer(&DiskReadWriteHistory, DiskBytesDelta);

    DiskBytesDelta = 0;

    return GetAverageCircularBuffer(&DiskReadWriteHistory);
}


UINT16
DiskGetResponseTime()
{
    return DiskResponseTime / 1000; //ms
}

