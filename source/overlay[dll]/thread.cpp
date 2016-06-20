#include <sl.h>
#include <stdio.h>

#include "thread.h"
#include "overlay.h"


typedef struct _THREAD_LIST THREAD_LIST_ENTRY;
typedef struct _THREAD_LIST {

    UINT16              ThreadId;
    UINT64              cycles;
    UINT8               cpuUsage;
    //VOID*               handle;
    BOOLEAN                affinitized;
    THREAD_LIST_ENTRY*  NextEntry;

} THREAD_LIST_ENTRY, *THREAD_LIST;
typedef struct _PROCESSOR_POWER_INFORMATION {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

THREAD_LIST_ENTRY* TmThreadList = 0;
VOID* TmHeapHandle;
ULONG MaxMhz;
extern UINT64 CyclesWaited;

#define ProcessorInformation 11
extern "C" NTSTATUS __stdcall CallNtPowerInformation(
    UINT32 InformationLevel,
    VOID*                   lpInputBuffer,
    ULONG                   nInputBufferSize,
    VOID*                   lpOutputBuffer,
    ULONG                   nOutputBufferSize
    );
#ifdef _WIN64
#include <intrin.h>

TEB* __stdcall NtCurrentTeb()
{
  return (TEB *) __readgsqword(0x30);
}
#endif


VOID
AddToThreadList( UINT16 ThreadId )
{
    THREAD_LIST_ENTRY *threadListEntry;
    OBJECT_ATTRIBUTES objectAttributes = {0};
    CLIENT_ID id = {0};
    VOID *handle = 0;

    //InitializeObjectAttributes(&objectAttributes, 0,0,0,0);
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);

    //clear struct
    //memset(&id, 0, sizeof(id));

    id.UniqueProcess = 0;
    id.UniqueThread = (VOID*)ThreadId;

    /*NtOpenThread(&handle,
                 THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION,
                 &objectAttributes,
                 &id);*/

    if (TmThreadList == 0)
    {
        //thread list is empty, mallocate first entry and return
        TmThreadList = (THREAD_LIST_ENTRY*) RtlAllocateHeap(
                                TmHeapHandle,
                                0,
                                sizeof(THREAD_LIST_ENTRY)
                                );

        TmThreadList->NextEntry     = 0;
        TmThreadList->affinitized   = 0;
        TmThreadList->cpuUsage      = 0;
        TmThreadList->cycles        = 0;
        //threadListFirstEntry->handle        = handle;
        TmThreadList->ThreadId = ThreadId;

        return;
    }

    threadListEntry = (THREAD_LIST_ENTRY*) TmThreadList;

    //move to empty list
    while(threadListEntry->NextEntry != 0)
    {
        //check if already added
        if (threadListEntry->ThreadId == ThreadId)
            return;

        threadListEntry = threadListEntry->NextEntry;
    }

    //mallocate memory for new thread entry
    threadListEntry->NextEntry = (THREAD_LIST_ENTRY*) RtlAllocateHeap(
                                                        TmHeapHandle,
                                                        0,
                                                        sizeof(THREAD_LIST_ENTRY)
                                                        );

    //move to newly mallocated memory
    threadListEntry = threadListEntry->NextEntry;

    //init entry
    threadListEntry->NextEntry      = 0;
    threadListEntry->affinitized    = 0;
    threadListEntry->cpuUsage       = 0;
    threadListEntry->cycles         = 0;
    //threadListEntry->handle         = handle;
    threadListEntry->ThreadId = ThreadId;
}


SYSTEM_PROCESS_INFORMATION* GetProcessInformation( UINT32* BufferSize )
{
    UINT64 delta = 0;
    UINT32 ProcOffset = 0;
    UINT32 bufferSize;
    PROCESSID processId;
    INT32 status = 0;
    VOID*           ProcThrdInfo = 0;
    SYSTEM_PROCESS_INFORMATION *processEntry;

    bufferSize = 0x4000;

    NtAllocateVirtualMemory(
        (VOID*)-1,
        &ProcThrdInfo,
        0,
        &bufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    while (TRUE)
    {
        status = NtQuerySystemInformation(
            SystemProcessInformation,
            ProcThrdInfo,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            NtFreeVirtualMemory((VOID*)-1, &ProcThrdInfo, (UINT32*)&bufferSize, MEM_RELEASE);

            ProcThrdInfo = 0;

            NtAllocateVirtualMemory(
                (VOID*)-1,
                &ProcThrdInfo,
                0,
                &bufferSize,
                MEM_COMMIT,
                PAGE_READWRITE
                );
        }
        else
        {
            break;
        }
    }

    processEntry = (SYSTEM_PROCESS_INFORMATION*)ProcThrdInfo;
    processId = (UINT32)NtCurrentTeb()->ClientId.UniqueProcess;

    do
    {
        processEntry = (SYSTEM_PROCESS_INFORMATION*)((UINT_B)processEntry + ProcOffset);

        if ((UINT16)processEntry->UniqueProcessId == processId)
        {
            break;
        }


        ProcOffset = processEntry->NextEntryOffset;

    } while (processEntry != 0 && processEntry->NextEntryOffset != 0);

    return processEntry;
}


ThreadMonitor::ThreadMonitor()
{
    UINT64 delta = 0;
    UINT32 ProcOffset = 0;
    UINT32 n, bufferSize;
    INT32 status = 0;
    SYSTEM_PROCESS_INFORMATION *processEntry;
    SYSTEM_THREAD_INFORMATION *threads;

    processEntry = GetProcessInformation(&bufferSize);

    threads = processEntry->Threads;
    TmHeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    for(n = 0; n < processEntry->NumberOfThreads; n++)
    {
        AddToThreadList( (UINT16) threads[n].ClientId.UniqueThread );
        printf("adding thread %u to list\n", (UINT16) threads[n].ClientId.UniqueThread);
    }

    NtFreeVirtualMemory(
        (VOID*)-1, 
        (VOID**)&processEntry, 
        (UINT32*)&bufferSize, 
        MEM_RELEASE
        );

    SYSTEM_BASIC_INFORMATION basicInfo;
    PPROCESSOR_POWER_INFORMATION ppi;

    NtQuerySystemInformation(
        SystemBasicInformation, 
        &basicInfo, 
        sizeof(SYSTEM_BASIC_INFORMATION), 
        0
        );

    const int size = basicInfo.NumberOfProcessors * sizeof(PROCESSOR_POWER_INFORMATION);
    ppi = (PPROCESSOR_POWER_INFORMATION) RtlAllocateHeap(
        NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,
        HEAP_ZERO_MEMORY,
        size
        );

    CallNtPowerInformation(ProcessorInformation, 0, 0, ppi, size);
    MaxMhz = ppi->MaxMhz;
}


VOID UpdateThreadList()
{
    SYSTEM_PROCESS_INFORMATION *processInfo;
    static UINT8 currentThreadCount;
    UINT8 n;
    UINT32 bufferSize;

    processInfo = GetProcessInformation(&bufferSize);
    
    if (currentThreadCount != processInfo->NumberOfThreads)
    {
        for (n = 0; n < processInfo->NumberOfThreads; n++)
        {
            AddToThreadList((UINT16)processInfo->Threads[n].ClientId.UniqueThread);
        }
    }

    NtFreeVirtualMemory(
        (VOID*)-1, 
        (VOID**)&processInfo, 
        (UINT32*)&bufferSize, 
        MEM_RELEASE
        );
}


VOID
ThreadMonitor::Refresh()
{
    UINT64 cyclesDeltaMax = 0;
    THREAD_LIST_ENTRY *thread, *previousEntry;
    NTSTATUS status;

    thread = (THREAD_LIST_ENTRY*) TmThreadList;

    while (thread != 0)
    {
        THREAD_CYCLE_TIME_INFORMATION cycles;
        OBJECT_ATTRIBUTES objectAttributes = {0};
        CLIENT_ID id = {0};
        VOID *handle = 0;

        objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);

        id.UniqueProcess = 0;
        id.UniqueThread = (VOID*) thread->ThreadId;

        status = NtOpenThread(
                    &handle,
                    THREAD_QUERY_INFORMATION,
                    &objectAttributes,
                    &id
                    );

        if(!NT_SUCCESS(status))
        {
            if (thread == TmThreadList)
                TmThreadList = thread->NextEntry;
            else
                previousEntry->NextEntry = thread->NextEntry;

            thread = thread->NextEntry;

            continue;
        }

        NtQueryInformationThread(
            handle,
            ThreadCycleTime,
            &cycles,
            sizeof(THREAD_CYCLE_TIME_INFORMATION),
            0
            );

        NtClose(handle);

        // Ensure that this function was already called at least once and we
        // have the previous cycle time value
        if (thread->cycles)
        {
            UINT64 cyclesDelta;

            cyclesDelta = cycles.AccumulatedCycles.QuadPart - thread->cycles;

            if (cyclesDelta > cyclesDeltaMax)
                cyclesDeltaMax = cyclesDelta;
        }

        thread->cycles = cycles.AccumulatedCycles.QuadPart;

        previousEntry = thread;
        thread = thread->NextEntry;
    }

    MaxThreadCyclesDelta = cyclesDeltaMax;
}


UINT8 ThreadMonitor::GetMaxThreadUsage()
{
    FLOAT threadUsage = 0.0f;

    //Remove waiting cycles used by frame limiter
    MaxThreadCyclesDelta -= CyclesWaited;
    CyclesWaited = 0;
    
    threadUsage = ((FLOAT)MaxThreadCyclesDelta / (FLOAT)(MaxMhz * 1000000)) * 100;

    //clip calculated thread usage to [0-100] range to filter calculation non-ideality

    if (threadUsage < 0.0f)
        threadUsage = 0.0f;

    if (threadUsage > 100.0f)
        threadUsage = 100.0f;

    return threadUsage;
}
