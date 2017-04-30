#include <sl.h>
#include <slprocess.h>
#include <string.h>
#include <push.h>


typedef struct _PROCESS_RESPONSE_TIME
{
    UINT32 ProcessId;
    UINT32 DiskResponseTime;
}PROCESS_RESPONSE_TIME;


UINT64 DiskBytesDelta;
UINT8 ResponseTimeProcessCount;
PROCESS_RESPONSE_TIME* ProcessResponseTimes;
LARGE_INTEGER PerformanceFrequency;


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
    VOID* FileObject;
    VOID* FileObject2;
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
typedef struct
{
    ULONG_PTR FileObject;
    WCHAR FileName[1];
} FileIo_Name;

#define EVENT_TRACE_REAL_TIME_MODE 0x00000100
#define KERNEL_LOGGER_NAME L"NT Kernel Logger"
#define EVENT_TRACE_FLAG_DISK_IO 0x00000100
#define EVENT_TRACE_FLAG_DISK_FILE_IO       0x00000200  // requires disk IO
#define WNODE_FLAG_TRACED_GUID   0x00020000
#define EVENT_TRACE_FILE_MODE_SEQUENTIAL 0x00000001
#define EVENT_TRACE_TYPE_IO_READ 0x0A
#define EVENT_TRACE_TYPE_IO_WRITE 0x0B

static GUID SystemTraceControlGuid_I = { 0x9e814aad, 0x3204, 0x11d2, { 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39 } };
static GUID DiskIoGuid = { 0x3d6fa8d4, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };
static GUID FileIoGuid = { 0x90cbdc39, 0x4a3e, 0x11d1, { 0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3 } };


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

    Buffer->Data = (UINT32*) Memory_Allocate(sizeof(ULONG) * Buffer->Size);
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


#include <sldebug.h>


/**
* A comparison function used by a hashtable.
*
* \param Entry1 The first entry.
* \param Entry2 The second entry.
*
* \return TRUE if the entries are equal, otherwise
* FALSE.
*/
typedef BOOLEAN(__stdcall *PPH_HASHTABLE_COMPARE_FUNCTION)(
    _In_ VOID* Entry1,
    _In_ VOID* Entry2
    );

/**
* A hash function used by a hashtable.
*
* \param Entry The entry.
*
* \return A hash code for the entry.
*
* \remarks
* \li Two entries which are considered to be equal
* by the comparison function must be given the same
* hash code.
* \li Two different entries do not have to be given
* different hash codes.
*/
typedef ULONG(__stdcall *PPH_HASHTABLE_HASH_FUNCTION)(
    _In_ VOID* Entry
    );


/**
* A hashtable structure.
*/
typedef struct _PH_HASHTABLE
{
    /** Size of user data in each entry. */
    ULONG EntrySize;
    /** The comparison function. */
    PPH_HASHTABLE_COMPARE_FUNCTION CompareFunction;
    /** The hash function. */
    PPH_HASHTABLE_HASH_FUNCTION HashFunction;

    /** The number of allocated buckets. */
    ULONG AllocatedBuckets;
    /** The bucket array. */
    ULONG* Buckets;
    /** The number of allocated entries. */
    ULONG AllocatedEntries;
    /** The entry array. */
    VOID* Entries;

    /** Number of entries in the hashtable. */
    ULONG Count;
    /** Index into entry array for free list. */
    ULONG FreeEntry;
    /** Index of next usable index into entry array, a.k.a. the
    * count of entries that were ever allocated.
    */
    ULONG NextEntry;
} PH_HASHTABLE, *PPH_HASHTABLE;
PPH_HASHTABLE EtFileNameHashtable;
// Simple hashtable

typedef struct _PH_KEY_VALUE_PAIR
{
    VOID* Key;
    VOID* Value;
} PH_KEY_VALUE_PAIR, *PPH_KEY_VALUE_PAIR;


// Basic types

typedef struct _QUAD
{
    union
    {
        __int64 UseThisFieldToCopy;
        double DoNotUseThisField;
    };
} QUAD, *PQUAD;


typedef struct _PH_HASHTABLE_ENTRY
{
    /** Hash code of the entry. -1 if entry is unused. */
    ULONG HashCode;
    /** Either the index of the next entry in the bucket,
    * the index of the next free entry, or -1 for invalid.
    */
    ULONG Next;
    /** The beginning of user data. */
    QUAD Body;
} PH_HASHTABLE_ENTRY, *PPH_HASHTABLE_ENTRY;

#define PTR_ADD_OFFSET(Pointer, Offset) ((VOID*)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))
#define FIELD_OFFSET(type, field)    ((LONG)(LONG)&(((type *)0)->field))
#define PH_HASHTABLE_ENTRY_SIZE(InnerSize) (FIELD_OFFSET(PH_HASHTABLE_ENTRY, Body) + (InnerSize))
#define PH_HASHTABLE_GET_ENTRY(Hashtable, Index) \
    ((PPH_HASHTABLE_ENTRY)PTR_ADD_OFFSET((Hashtable)->Entries, \
    PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize) * (Index)))

// Use power-of-two sizes instead of primes
#define PH_HASHTABLE_POWER_OF_TWO_SIZE


ULONG PhpIndexFromHash(
    _In_ PPH_HASHTABLE Hashtable,
    _In_ ULONG Hash
    )
{
#ifdef PH_HASHTABLE_POWER_OF_TWO_SIZE
    return Hash & (Hashtable->AllocatedBuckets - 1);
#else
    return Hash % Hashtable->AllocatedBuckets;
#endif
}


/**
* Rounds up a number to the next power of two.
*/
ULONG PhRoundUpToPowerOfTwo(
    _In_ ULONG Number
    )
{
    Number--;
    Number |= Number >> 1;
    Number |= Number >> 2;
    Number |= Number >> 4;
    Number |= Number >> 8;
    Number |= Number >> 16;
    Number++;

    return Number;
}


ULONG PhpGetNumberOfBuckets(
    _In_ ULONG Capacity
    )
{
#ifdef PH_HASHTABLE_POWER_OF_TWO_SIZE
    return PhRoundUpToPowerOfTwo(Capacity);
#else
    return PhGetPrimeNumber(Capacity);
#endif
}


VOID PhpResizeHashtable(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ ULONG NewCapacity
    )
{
    PPH_HASHTABLE_ENTRY entry;
    ULONG i;

    // Re-allocate the buckets. Note that we don't need to keep the
    // contents.
    Hashtable->AllocatedBuckets = PhpGetNumberOfBuckets(NewCapacity);
    Memory_Free(Hashtable->Buckets);
    Hashtable->Buckets = (ULONG*) Memory_Allocate(sizeof(ULONG) * Hashtable->AllocatedBuckets);
    // Set all bucket values to -1.
    memset(Hashtable->Buckets, 0xff, sizeof(ULONG) * Hashtable->AllocatedBuckets);

    // Re-allocate the entries.
    Hashtable->AllocatedEntries = Hashtable->AllocatedBuckets;
    Hashtable->Entries = Memory_ReAllocate(
        Hashtable->Entries,
        PH_HASHTABLE_ENTRY_SIZE(Hashtable->EntrySize) * Hashtable->AllocatedEntries
        );

    // Re-distribute the entries among the buckets.

    // PH_HASHTABLE_GET_ENTRY is quite slow (it involves a multiply), so we use a pointer here.
    entry = (PH_HASHTABLE_ENTRY*) Hashtable->Entries;

    for (i = 0; i < Hashtable->NextEntry; i++)
    {
        if (entry->HashCode != -1)
        {
            ULONG index = PhpIndexFromHash(Hashtable, entry->HashCode);

            entry->Next = Hashtable->Buckets[index];
            Hashtable->Buckets[index] = i;
        }

        entry = (PPH_HASHTABLE_ENTRY)((ULONG_PTR)entry + PH_HASHTABLE_ENTRY_SIZE(Hashtable->EntrySize));
    }
}


#define MAXLONG     0x7fffffff


ULONG PhpValidateHash(
    _In_ ULONG Hash
    )
{
    // No point in using a full hash when we're going to
    // AND with size minus one anyway.
#if defined(PH_HASHTABLE_FULL_HASH) && !defined(PH_HASHTABLE_POWER_OF_TWO_SIZE)
    if (Hash != -1)
        return Hash;
    else
        return 0;
#else
    return Hash & MAXLONG;
#endif
}


VOID* PhpAddEntryHashtable(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ VOID* Entry,
    _In_ BOOLEAN CheckForDuplicate,
    _Out_opt_ BOOLEAN* Added
    )
{
    ULONG hashCode; // hash code of the new entry
    ULONG index; // bucket index of the new entry
    ULONG freeEntry; // index of new entry in entry array
    PPH_HASHTABLE_ENTRY entry; // pointer to new entry in entry array

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = PhpIndexFromHash(Hashtable, hashCode);

    if (CheckForDuplicate)
    {
        ULONG i;

        for (i = Hashtable->Buckets[index]; i != -1; i = entry->Next)
        {
            entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

            if (entry->HashCode == hashCode && Hashtable->CompareFunction(&entry->Body, Entry))
            {
                if (Added)
                    *Added = FALSE;

                return &entry->Body;
            }
        }
    }

    // Use a free entry if possible.
    if (Hashtable->FreeEntry != -1)
    {
        freeEntry = Hashtable->FreeEntry;
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, freeEntry);
        Hashtable->FreeEntry = entry->Next;
    }
    else
    {
        // Use the next entry in the entry array.

        if (Hashtable->NextEntry == Hashtable->AllocatedEntries)
        {
            // Resize the hashtable.
            PhpResizeHashtable(Hashtable, Hashtable->AllocatedBuckets * 2);
            index = PhpIndexFromHash(Hashtable, hashCode);
        }

        freeEntry = Hashtable->NextEntry++;
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, freeEntry);
    }

    // Initialize the entry.
    entry->HashCode = hashCode;
    entry->Next = Hashtable->Buckets[index];
    Hashtable->Buckets[index] = freeEntry;
    // Copy the user-supplied data to the entry.
    memcpy(&entry->Body, Entry, Hashtable->EntrySize);

    Hashtable->Count++;

    if (Added)
        *Added = TRUE;

    return &entry->Body;
}


/**
* Adds an entry to a hashtable or returns an existing one.
*
* \param Hashtable A hashtable object.
* \param Entry The entry to add.
* \param Added A variable which receives TRUE if a new
* entry was created, and FALSE if an existing entry was
* returned.
*
* \return A pointer to the entry as stored in
* the hashtable. This pointer is valid until
* the hashtable is modified. If the hashtable
* already contained an equal entry, the existing entry
* is returned. Check the value of \a Added to determine
* whether the returned entry is new or existing.
*
* \remarks Entries are only guaranteed to be 8 byte
* aligned, even on 64-bit systems.
*/
VOID* PhAddEntryHashtableEx(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ VOID* Entry,
    _Out_opt_ BOOLEAN* Added
    )
{
    return PhpAddEntryHashtable(Hashtable, Entry, TRUE, Added);
}


typedef struct _PH_STRING *PPH_STRING;


/**
* Locates an entry in a hashtable.
*
* \param Hashtable A hashtable object.
* \param Entry An entry representing the
* entry to find.
*
* \return A pointer to the entry as stored in
* the hashtable. This pointer is valid until
* the hashtable is modified. If the entry
* could not be found, NULL is returned.
*
* \remarks The entry specified in \a Entry
* can be a partial entry that is filled in enough
* so that the comparison and hash functions can
* work with them.
*/
VOID* PhFindEntryHashtable(
    _In_ PPH_HASHTABLE Hashtable,
    _In_ VOID* Entry
    )
{
    ULONG hashCode;
    ULONG index;
    ULONG i;
    PPH_HASHTABLE_ENTRY entry;

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = PhpIndexFromHash(Hashtable, hashCode);

    for (i = Hashtable->Buckets[index]; i != -1; i = entry->Next)
    {
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

        if (entry->HashCode == hashCode && Hashtable->CompareFunction(&entry->Body, Entry))
        {
            return &entry->Body;
        }
    }

    return NULL;
}


PPH_STRING EtFileObjectToFileName(
    _In_ VOID* FileObject
    )
{
    PH_KEY_VALUE_PAIR pair;
    PPH_KEY_VALUE_PAIR realPair;
    PPH_STRING fileName;

    pair.Key = FileObject;
    fileName = NULL;

    realPair = (PH_KEY_VALUE_PAIR*) PhFindEntryHashtable(EtFileNameHashtable, &pair);

    if (realPair)
    {
        Log((WCHAR*)realPair->Value);
    }

    return fileName;
}


BOOLEAN __stdcall PhpSimpleHashtableCompareFunction(
    _In_ VOID* Entry1,
    _In_ VOID* Entry2
    )
{
    PPH_KEY_VALUE_PAIR entry1 = (PH_KEY_VALUE_PAIR*) Entry1;
    PPH_KEY_VALUE_PAIR entry2 = (PH_KEY_VALUE_PAIR*) Entry2;

    return entry1->Key == entry2->Key;
}


ULONG PhHashInt32(
_In_ ULONG Value
)
{
    // Java style.
    Value ^= (Value >> 20) ^ (Value >> 12);
    return Value ^ (Value >> 7) ^ (Value >> 4);
}


ULONG PhHashIntPtr(
_In_ ULONG_PTR Value
)
{
#ifdef _WIN64
    return PhHashInt64(Value);
#else
    return PhHashInt32(Value);
#endif
}


ULONG __stdcall PhpSimpleHashtableHashFunction(
    _In_ VOID* Entry
    )
{
    PPH_KEY_VALUE_PAIR entry = (PH_KEY_VALUE_PAIR*) Entry;

    return PhHashIntPtr((ULONG_PTR)entry->Key);
}


struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;
PPH_OBJECT_TYPE PhHashtableType;


/**
* Creates a hashtable object.
*
* \param EntrySize The size of each hashtable entry,
* in bytes.
* \param CompareFunction A comparison function that
* is executed to compare two hashtable entries.
* \param HashFunction A hash function that is executed
* to generate a hash code for a hashtable entry.
* \param InitialCapacity The number of entries to
* allocate storage for initially.
*/
PPH_HASHTABLE PhCreateHashtable(
    _In_ ULONG EntrySize,
    _In_ PPH_HASHTABLE_COMPARE_FUNCTION CompareFunction,
    _In_ PPH_HASHTABLE_HASH_FUNCTION HashFunction,
    _In_ ULONG InitialCapacity
    )
{
    PPH_HASHTABLE hashtable;

    hashtable = (PH_HASHTABLE*) Memory_Allocate(sizeof(PH_HASHTABLE));

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    hashtable->EntrySize = EntrySize;
    hashtable->CompareFunction = CompareFunction;
    hashtable->HashFunction = HashFunction;

    // Allocate the buckets.
    hashtable->AllocatedBuckets = PhpGetNumberOfBuckets(InitialCapacity);
    hashtable->Buckets = (ULONG*) Memory_Allocate(sizeof(ULONG) * hashtable->AllocatedBuckets);
    // Set all bucket values to -1.
    memset(hashtable->Buckets, 0xff, sizeof(ULONG) * hashtable->AllocatedBuckets);

    // Allocate the entries.
    hashtable->AllocatedEntries = hashtable->AllocatedBuckets;
    hashtable->Entries = Memory_Allocate(PH_HASHTABLE_ENTRY_SIZE(EntrySize) * hashtable->AllocatedEntries);

    hashtable->Count = 0;
    hashtable->FreeEntry = -1;
    hashtable->NextEntry = 0;

    return hashtable;
}


PPH_HASHTABLE PhCreateSimpleHashtable(
    _In_ ULONG InitialCapacity
    )
{
    return PhCreateHashtable(
        sizeof(PH_KEY_VALUE_PAIR),
        PhpSimpleHashtableCompareFunction,
        PhpSimpleHashtableHashFunction,
        InitialCapacity
        );
}


VOID __stdcall DiskEvents( EVENT_TRACE* EventTrace )
{
    DiskIo_TypeGroup1 *data = (DiskIo_TypeGroup1*)EventTrace->MofData;

    if (EventTrace->Header.Class.Type == EVENT_TRACE_TYPE_IO_READ
        || EventTrace->Header.Class.Type == EVENT_TRACE_TYPE_IO_WRITE)
    {
        UINT32 responseTime;
        PROCESS_RESPONSE_TIME *processResponseTime;
        int i;
        BOOLEAN inArray = FALSE;

        DiskBytesDelta += data->TransferSize;
        responseTime = (FLOAT)data->HighResResponseTime * 1000 / PerformanceFrequency.QuadPart;

        if (!ProcessResponseTimes)
        {
            ProcessResponseTimes = (PROCESS_RESPONSE_TIME*)Memory_Allocate(sizeof(PROCESS_RESPONSE_TIME));

            Memory_Clear(ProcessResponseTimes, sizeof(PROCESS_RESPONSE_TIME));
            
            ResponseTimeProcessCount++;
        }

parse:
        for (i = 0, processResponseTime = ProcessResponseTimes; i < ResponseTimeProcessCount; i++, processResponseTime++)
        {
            if (!processResponseTime->ProcessId)
            {
                processResponseTime->ProcessId = EventTrace->Header.ProcessId;
                processResponseTime->DiskResponseTime = responseTime;
                inArray = TRUE;
            }   
            else if (processResponseTime->ProcessId == EventTrace->Header.ProcessId)
            {
                inArray = TRUE;

                if (responseTime > processResponseTime->DiskResponseTime)
                    processResponseTime->DiskResponseTime = responseTime;
            }
        }
            
        if (!inArray)
        {
            ProcessResponseTimes = (PROCESS_RESPONSE_TIME*)Memory_ReAllocate(
                ProcessResponseTimes,
                sizeof(PROCESS_RESPONSE_TIME) * (ResponseTimeProcessCount + 1)
                );

            Memory_Clear(ProcessResponseTimes + ResponseTimeProcessCount, sizeof(PROCESS_RESPONSE_TIME));

            ResponseTimeProcessCount++;

            goto parse;
        }
        
        if (responseTime > 4000 || PushSharedMemory->LogFileIo)
        {
            EtFileObjectToFileName(data->FileObject);

            if (PushSharedMemory->AutoLogFileIo)
                PushSharedMemory->LogFileIo = FALSE;
        }
            
    }
}


VOID __stdcall FileEvents( EVENT_TRACE* EventTrace )
{
    FileIo_Name *data;
    PH_KEY_VALUE_PAIR pair;
    PPH_KEY_VALUE_PAIR realPair;

    //if (EventTrace->Header.ProcessId == GameProcessId)
    //{
        data = (FileIo_Name*)EventTrace->MofData;

        pair.Key = (VOID*)data->FileObject;
        pair.Value = Memory_Allocate(String_GetSize(data->FileName) + 1);

        String_Copy((WCHAR*)pair.Value, data->FileName);

        realPair = (PH_KEY_VALUE_PAIR*)PhAddEntryHashtableEx(EtFileNameHashtable, &pair, NULL);
    //}
}


DWORD __stdcall DiskMonitorThread( VOID* Parameter )
{
    EVENT_TRACE_PROPERTIES *traceProperties;
    UINT32 bufferSize;
    EVENT_TRACE_LOGFILEW trace = { 0 };
    UINT64 traceHandle = NULL, sessionHandle = NULL;

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAME);
    traceProperties = (EVENT_TRACE_PROPERTIES*) Memory_Allocate(bufferSize);

    memset(traceProperties, 0, sizeof(EVENT_TRACE_PROPERTIES));

    traceProperties->Wnode.BufferSize = bufferSize;
    traceProperties->Wnode.Guid = SystemTraceControlGuid_I;
    traceProperties->Wnode.ClientContext = 1;
    traceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    traceProperties->MinimumBuffers = 1;
    traceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    traceProperties->FlushTimer = 1;
    traceProperties->EnableFlags = EVENT_TRACE_FLAG_DISK_IO | EVENT_TRACE_FLAG_DISK_FILE_IO;
    traceProperties->LogFileNameOffset = 0;
    traceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    StartTraceW(&sessionHandle, KERNEL_LOGGER_NAME, traceProperties);

    trace.LoggerName = KERNEL_LOGGER_NAME;
    trace.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

    traceHandle = OpenTraceW(&trace);

    SetTraceCallback(&DiskIoGuid, DiskEvents);
    SetTraceCallback(&FileIoGuid, FileEvents);

    ProcessTrace(&traceHandle, 1, 0, 0);

    return 0;
}


VOID DiskStartMonitoring()
{
    LARGE_INTEGER performanceCounter;

    NtQueryPerformanceCounter(&performanceCounter, &PerformanceFrequency);
    InitializeCircularBuffer(&DiskReadWriteHistory, 2);
    EtFileNameHashtable = PhCreateSimpleHashtable(128);
    CreateRemoteThread(NtCurrentProcess(), 0, 0, &DiskMonitorThread, 0, 0, 0);
}


UINT64
DiskGetBytesDelta()
{
    AddItemCircularBuffer(&DiskReadWriteHistory, DiskBytesDelta);

    DiskBytesDelta = 0;

    return GetAverageCircularBuffer(&DiskReadWriteHistory);
}


/*VOID DiskGetResponseTimes( PROCESS_RESPONSE_TIME** ResponseTimes, UINT8* ResponseTimesCount )
{
    PROCESS_RESPONSE_TIME *responseTimes = (PROCESS_RESPONSE_TIME*) Memory_Allocate(
        sizeof(PROCESS_RESPONSE_TIME) * ResponseTimeProcessCount
        );
    
    Memory_Copy(responseTimes, ProcessResponseTimes, sizeof(PROCESS_RESPONSE_TIME) * ResponseTimeProcessCount);

    *ResponseTimes = responseTimes;
    *ResponseTimesCount = ResponseTimeProcessCount;
}*/


UINT16 DiskGetResponseTime( UINT32 ProcessId )
{
    PROCESS_RESPONSE_TIME *responseTime;
    int i;

    for (i = 0, responseTime = ProcessResponseTimes; i < ResponseTimeProcessCount; i++, responseTime++)
    {
        if (responseTime->ProcessId == ProcessId)
            return responseTime->DiskResponseTime;
    }

    return 0;
}

