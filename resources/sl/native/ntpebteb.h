#ifndef _NTPEBTEB_H
#define _NTPEBTEB_H

#include "ntpsapi.h"
#include "ntrtl.h"


typedef struct _PEB
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    union
    {
        BOOLEAN BitField;
        struct
        {
            BOOLEAN ImageUsesLargePages : 1;
            BOOLEAN IsProtectedProcess : 1;
            BOOLEAN IsImageDynamicallyRelocated : 1;
            BOOLEAN SkipPatchingUser32Forwarders : 1;
            BOOLEAN IsPackagedProcess : 1;
            BOOLEAN IsAppContainer : 1;
            BOOLEAN IsProtectedProcessLight : 1;
            BOOLEAN SpareBits : 1;
        };
    };
    VOID* Mutant;

    VOID* ImageBaseAddress;
    PEB_LDR_DATA* Ldr;
    RTL_USER_PROCESS_PARAMETERS* ProcessParameters;
    VOID* SubSystemData;
    VOID* ProcessHeap;
    RTL_CRITICAL_SECTION* FastPebLock;
    VOID* AtlThunkSListPtr;
    VOID* IFEOKey;
    union
    {
        ULONG CrossProcessFlags;
        struct
        {
            ULONG ProcessInJob : 1;
            ULONG ProcessInitializing : 1;
            ULONG ProcessUsingVEH : 1;
            ULONG ProcessUsingVCH : 1;
            ULONG ProcessUsingFTH : 1;
            ULONG ReservedBits0 : 27;
        };
        ULONG EnvironmentUpdateCount;
    };
    union
    {
        VOID* KernelCallbackTable;
        VOID* UserSharedInfoPtr;
    };
    ULONG SystemReserved[1];
    ULONG AtlThunkSListPtr32;
    VOID* ApiSetMap;
    ULONG TlsExpansionCounter;
    VOID* TlsBitmap;
    ULONG TlsBitmapBits[2];
    VOID* ReadOnlySharedMemoryBase;
    VOID* HotpatchInformation;
    VOID** ReadOnlyStaticServerData;
    VOID* AnsiCodePageData;
    VOID* OemCodePageData;
    VOID* UnicodeCaseTableData;

    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;

    LARGE_INTEGER CriticalSectionTimeout;
    UINT_B HeapSegmentReserve;
    UINT_B HeapSegmentCommit;
    UINT_B HeapDeCommitTotalFreeThreshold;
    UINT_B HeapDeCommitFreeBlockThreshold;

    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    VOID** ProcessHeaps;

    VOID* GdiSharedHandleTable;
    VOID* ProcessStarterHelper;
    ULONG GdiDCAttributeList;

    RTL_CRITICAL_SECTION* LoaderLock;

    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    UINT16 OSBuildNumber;
    UINT16 OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    SIZE_B ImageProcessAffinityMask;
    GDI_HANDLE_BUFFER GdiHandleBuffer;
    VOID* PostProcessInitRoutine;

    VOID* TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];

    ULONG SessionId;

    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    VOID* pShimData;
    VOID* AppCompatInfo;

    UNICODE_STRING CSDVersion;

    VOID* ActivationContextData;
    VOID* ProcessAssemblyStorageMap;
    VOID* SystemDefaultActivationContextData;
    VOID* SystemAssemblyStorageMap;

    UINT_B MinimumStackCommit;

    VOID** FlsCallback;
    LIST_ENTRY FlsListHead;
    VOID* FlsBitmap;
    ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)];
    ULONG FlsHighIndex;

    VOID* WerRegistrationData;
    VOID* WerShipAssertPtr;
    VOID* pContextData;
    VOID* pImageHeaderHash;
    union
    {
        ULONG TracingFlags;
        struct
        {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG LibLoaderTracingEnabled : 1;
            ULONG SpareTracingBits : 29;
        };
    };
    UINT64 CsrServerReadOnlySharedMemoryBase;
} PEB;

#define GDI_BATCH_BUFFER_SIZE 310

typedef struct _GDI_TEB_BATCH
{
    ULONG Offset;
    UINT_B HDC;
    ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT
{
    ULONG Flags;
    CHAR* FrameName;
} TEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME
{
    ULONG Flags;
    struct _TEB_ACTIVE_FRAME *Previous;
    TEB_ACTIVE_FRAME_CONTEXT* Context;
} TEB_ACTIVE_FRAME;

typedef struct _TEB
{
    NT_TIB NtTib;

    VOID* EnvironmentPointer;
    CLIENT_ID ClientId;
    VOID* ActiveRpcHandle;
    VOID* ThreadLocalStoragePointer;
    PEB* ProcessEnvironmentBlock;

    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    VOID* CsrClientThread;
    VOID* Win32ThreadInfo;
    ULONG User32Reserved[26];
    ULONG UserReserved[5];
    VOID* WOW32Reserved;
    UINT32 CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    VOID* SystemReserved1[54];
    UINT32 ExceptionCode;
    VOID* ActivationContextStackPointer;
#ifdef _M_X64
    BYTE SpareBytes[24];
#else
    BYTE SpareBytes[36];
#endif
    ULONG TxFsContext;

    GDI_TEB_BATCH GdiTebBatch;
    CLIENT_ID RealClientId;
    VOID* GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    VOID* GdiThreadLocalInfo;
    UINT_B Win32ClientInfo[62];
    VOID* glDispatchTable[233];
    UINT_B glReserved1[29];
    VOID* glReserved2;
    VOID* glSectionInfo;
    VOID* glSection;
    VOID* glTable;
    VOID* glCurrentRC;
    VOID* glContext;

    UINT32 LastStatusValue;
    UNICODE_STRING StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[261];

    VOID* DeallocationStack;
    VOID* TlsSlots[64];
    LIST_ENTRY TlsLinks;

    VOID* Vdm;
    VOID* ReservedForNtRpc;
    VOID* DbgSsReserved[2];

    ULONG HardErrorMode;
#ifdef _M_X64
    VOID* Instrumentation[11];
#else
    VOID* Instrumentation[9];
#endif
    GUID ActivityId;

    VOID* SubProcessTag;
    VOID* EtwLocalData;
    VOID* EtwTraceData;
    VOID* WinSockData;
    ULONG GdiBatchCount;

    union
    {
        PROCESSOR_NUMBER CurrentIdealProcessor;
        ULONG IdealProcessorValue;
        struct
        {
            BYTE ReservedPad0;
            BYTE ReservedPad1;
            BYTE ReservedPad2;
            BYTE IdealProcessor;
        };
    };

    ULONG GuaranteedStackBytes;
    VOID* ReservedForPerf;
    VOID* ReservedForOle;
    ULONG WaitingOnLoaderLock;
    VOID* SavedPriorityState;
    UINT_B SoftPatchPtr1;
    VOID* ThreadPoolData;
    VOID** TlsExpansionSlots;
#ifdef _M_X64
    VOID* DeallocationBStore;
    VOID* BStoreLimit;
#endif
    ULONG MuiGeneration;
    ULONG IsImpersonating;
    VOID* NlsCache;
    VOID* pShimData;
    ULONG HeapVirtualAffinity;
    VOID* CurrentTransactionHandle;
    TEB_ACTIVE_FRAME* ActiveFrame;
    VOID* FlsData;

    VOID* PreferredLanguages;
    VOID* UserPrefLanguages;
    VOID* MergedPrefLanguages;
    ULONG MuiImpersonation;

    union
    {
        UINT16 CrossTebFlags;
        UINT16 SpareCrossTebBits : 16;
    };
    union
    {
        UINT16 SameTebFlags;
        struct
        {
            UINT16 SafeThunkCall : 1;
            UINT16 InDebugPrint : 1;
            UINT16 HasFiberData : 1;
            UINT16 SkipThreadAttach : 1;
            UINT16 WerInShipAssertCode : 1;
            UINT16 RanProcessInit : 1;
            UINT16 ClonedThread : 1;
            UINT16 SuppressDebugMsg : 1;
            UINT16 DisableUserStackWalk : 1;
            UINT16 RtlExceptionAttached : 1;
            UINT16 InitialThread : 1;
            UINT16 SessionAware : 1;
            UINT16 SpareSameTebBits : 4;
        };
    };

    VOID* TxnScopeEnterCallback;
    VOID* TxnScopeExitCallback;
    VOID* TxnScopeContext;
    ULONG LockCount;
    ULONG SpareUlong0;
    VOID* ResourceRetValue;
    VOID* ReservedForWdf;
} TEB;

#endif //_NTPEBTEB_H
