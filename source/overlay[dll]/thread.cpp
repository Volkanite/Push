#include <sltypes.h>
#include <slntuapi.h>
#include <sldetours.h>
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


typedef VOID* (__stdcall* TYPE_CreateThread)(
    SECURITY_ATTRIBUTES* lpThreadAttributes,
    UINT_B dwStackSize,
    VOID* lpStartAddress,
    VOID* lpParameter,
    DWORD dwCreationFlags,
    DWORD* lpThreadId
);


TYPE_CreateThread       TmCreateThread;
THREAD_LIST_ENTRY* TmThreadList = 0;
VOID* TmHeapHandle;

VOID
AddToThreadList( UINT16 threadID )
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
    id.UniqueThread = (VOID*) threadID;

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
        TmThreadList->ThreadId      = threadID;

        return;
    }

    threadListEntry = (THREAD_LIST_ENTRY*) TmThreadList;

    //move to empty list
    while(threadListEntry->NextEntry != 0)
    {
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
    threadListEntry->ThreadId       = threadID;
}


VOID* __stdcall CreateThreadHook(
    SECURITY_ATTRIBUTES* ThreadAttributes,
    UINT_B StackSize,
    VOID* StartAddress,
    VOID* Parameter,
    DWORD CreationFlags,
    DWORD* ThreadId
    )
{
    VOID* threadHandle;
    DWORD threadId = 0;

    threadHandle = TmCreateThread(
                    ThreadAttributes,
                    StackSize,
                    StartAddress,
                    Parameter,
                    CreationFlags,
                    &threadId
                    );

    printf("new thread : %u\n", threadId);

    AddToThreadList( threadId );

    if (ThreadId)
        *ThreadId = threadId;

    return threadHandle;
}



/*VOID
DestroyThreadList()
{
    THREAD_LIST_ENTRY *threadListEntry = 0;

    threadListEntry = threadListFirstEntry;

    while (threadListEntry != 0)
    {
        THREAD_LIST_ENTRY *nextEntry;

        nextEntry = threadListEntry->nextEntry;

        //SlFree(threadListEntry);
        RtlFreeHeap(
            PushHeapHandle,
            0,
            threadListEntry
            );

        threadListEntry = nextEntry;
    }

    threadListFirstEntry = 0;
}*/


extern "C"
DWORD
__stdcall
GetCurrentThreadId(
    VOID
    );

VOID
Swap( THREAD_LIST_ENTRY* a,
      THREAD_LIST_ENTRY* b )
{
    THREAD_LIST_ENTRY threadListEntry;

    threadListEntry.affinitized = a->affinitized;
    threadListEntry.cpuUsage    = a->cpuUsage;
    threadListEntry.cycles      = a->cycles;
    //threadListEntry.handle      = a->handle;
    threadListEntry.ThreadId    = a->ThreadId;

    a->affinitized  = b->affinitized;
    a->cpuUsage     = b->cpuUsage;
    a->cycles       = b->cycles;
    //a->handle       = b->handle;
    a->ThreadId     = b->ThreadId;

    b->affinitized  = threadListEntry.affinitized;
    b->cpuUsage     = threadListEntry.cpuUsage;
    b->cycles       = threadListEntry.cycles;
    //b->handle       = threadListEntry.handle;
    b->ThreadId     = threadListEntry.ThreadId;
}


VOID
Sort( THREAD_LIST_ENTRY* start )
{
    int swapped;
    THREAD_LIST_ENTRY *ptr1, *lptr = 0;

    do
    {
        swapped = 0;
        ptr1 = start;

        while (ptr1->NextEntry != lptr)
        {
            if (ptr1->cycles < ptr1->NextEntry->cycles)
            {
                Swap(ptr1, ptr1->NextEntry);
                swapped = 1;
            }
            ptr1 = ptr1->NextEntry;
        }
        lptr = ptr1;
    }
    while (swapped);
}


ThreadMonitor::ThreadMonitor()
{
    UINT64 delta = 0;
    UINT32 ProcOffset = 0;
    UINT32 n, bufferSize;
    UINT32 processId;
    INT32 status = 0;
    VOID*           ProcThrdInfo = 0;
    SYSTEM_PROCESS_INFORMATION *processEntry;
    SYSTEM_THREAD_INFORMATION *threads;
    SlHookManager hookManager;

    TmCreateThread = (TYPE_CreateThread) hookManager.DetourApi(
                        L"kernel32.dll",
                        "CreateThread",
                        (BYTE*)CreateThreadHook
                        );

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

        if(status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            NtFreeVirtualMemory((VOID*)-1, &ProcThrdInfo,(UINT32*)&bufferSize, MEM_RELEASE);

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
    processId = (UINT32) NtCurrentTeb()->ClientId.UniqueProcess;

    do
    {
      processEntry = (SYSTEM_PROCESS_INFORMATION*)((UINT_B)processEntry + ProcOffset);

      if ((UINT16)processEntry->UniqueProcessId == processId)
      {
          printf("processEntry->UniqueProcessId [%u] matches processId [%u]\n", processEntry->UniqueProcessId, processId);
          break;
      }


     ProcOffset = processEntry->NextEntryOffset;

    } while(processEntry != 0 && processEntry->NextEntryOffset != 0);

    threads = processEntry->Threads;
    TmHeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    for(n = 0; n < processEntry->NumberOfThreads; n++)
    {
        AddToThreadList( (UINT16) threads[n].ClientId.UniqueThread );
        printf("adding thread %u to list\n", (UINT16) threads[n].ClientId.UniqueThread);
    }

    NtFreeVirtualMemory((VOID*)-1, &ProcThrdInfo, (UINT32*)&bufferSize, MEM_RELEASE);
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


UINT8
ThreadMonitor::GetMaxThreadUsage()
{
    FLOAT threadUsage = 0.0f;

    threadUsage = (MaxThreadCyclesDelta / 100000000) * PushSharedMemory->HarwareInformation.Processor.NumberOfCores;

    //clip calculated thread usage to [0-100] range to filter calculation non-ideality

    if (threadUsage < 0.0f)
        threadUsage = 0.0f;

    if (threadUsage > 100.0f)
        threadUsage = 100.0f;

    return threadUsage;
}


/////////////////////////////////////////////////////////////////
// give first [cores - 1] threads sorted by cycle time
// their own core which minimizes context switches
/////////////////////////////////////////////////////////////////
VOID
ThreadMonitor::OptimizeThreads()
{
    THREAD_LIST_ENTRY *thread, *previousEntry;
    UINT32 i = 0;
    BOOLEAN affinityChanged = FALSE;
    NTSTATUS status;
    VOID *threadHandle;

    // Sort thread list by cycles
    Sort(TmThreadList);

    thread = TmThreadList;

    // Give first [cores - 1] threads their own core
    for(i = 0; i < PushSharedMemory->HarwareInformation.Processor.NumberOfCores - 1; i++)
    {
        if (thread->affinitized == FALSE)
        {
            OBJECT_ATTRIBUTES objectAttributes = {0};
            CLIENT_ID id = {0};

            objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);

            id.UniqueProcess = 0;
            id.UniqueThread = (VOID*) thread->ThreadId;

            status = NtOpenThread(
                    &threadHandle,
                    THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION,
                    &objectAttributes,
                    &id
                    );

            if(!NT_SUCCESS(status))
            {
                printf("cannot get information for thread %u\n", thread->ThreadId);

                if (thread == TmThreadList)
                    TmThreadList = thread->NextEntry;
                else
                    previousEntry->NextEntry = thread->NextEntry;

                thread = thread->NextEntry;

                continue;
            }

            SetThreadAffinityMask(threadHandle, 1<<i);
            NtClose(threadHandle);

            thread->affinitized = TRUE;
            affinityChanged = TRUE;
        }

        if (thread->NextEntry == 0)
            break;

        previousEntry = thread;
        thread = thread->NextEntry;
    }

    // Set other threads back to normal. Can be easily modified to set other threads to
    // last core but they seem more happy being able to choose which core to run on.
    // Also, piling a ton of threads on one core probably isn't such a good idea.
    if (affinityChanged)
    {
        while (thread != 0)
        {
            OBJECT_ATTRIBUTES objectAttributes = {0};
            CLIENT_ID id = {0};

            objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);

            id.UniqueProcess = 0;
            id.UniqueThread = (VOID*) thread->ThreadId;

            NtOpenThread(
                &threadHandle,
                THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION,
                &objectAttributes,
                &id
                );

            SetThreadAffinityMask(
                threadHandle,
                (1 << PushSharedMemory->HarwareInformation.Processor.NumberOfCores) - 1
                );

            NtClose(threadHandle);

            thread->affinitized = FALSE;
            thread = thread->NextEntry;
        }
    }
}
