#include <sl.h>


typedef struct _STARTUPINFOW {
    DWORD   cb;
    WCHAR*  lpReserved;
    WCHAR*  lpDesktop;
    WCHAR*  lpTitle;
    DWORD   dwX;
    DWORD   dwY;
    DWORD   dwXSize;
    DWORD   dwYSize;
    DWORD   dwXCountChars;
    DWORD   dwYCountChars;
    DWORD   dwFillAttribute;
    DWORD   dwFlags;
    WORD    wShowWindow;
    WORD    cbReserved2;
    BYTE*  lpReserved2;
    HANDLE  hStdInput;
    HANDLE  hStdOutput;
    HANDLE  hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;


INTBOOL __stdcall CreateProcessW(
    WCHAR* lpApplicationName,
    WCHAR* lpCommandLine,
    SECURITY_ATTRIBUTES* lpProcessAttributes,
    SECURITY_ATTRIBUTES* lpThreadAttributes,
    INTBOOL bInheritHandles,
    DWORD dwCreationFlags,
    VOID* lpEnvironment,
    WCHAR* lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );

NTSTATUS __stdcall NtWriteVirtualMemory(
    HANDLE ProcessHandle,
    VOID* BaseAddress,
    VOID* Buffer,
    SIZE_T BufferSize,
    SIZE_T* NumberOfBytesWritten
    );

NTSTATUS __stdcall NtFlushInstructionCache(
    HANDLE ProcessHandle,
    VOID* BaseAddress,
    SIZE_T Length
    );


VOID __stdcall RtlInitUnicodeString(
    UNICODE_STRING* DestinationString,
    WCHAR* SourceString
);


VOID* Process_Open( UINT32 processID, DWORD rights )
{
    OBJECT_ATTRIBUTES objAttrib;
    CLIENT_ID id = {0};
    VOID *processHandle = 0;

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = NULL;
    objAttrib.ObjectName = NULL;
    objAttrib.Attributes = 0;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    id.UniqueProcess = (HANDLE) processID;

    NtOpenProcess(&processHandle, rights, &objAttrib, &id);

    return processHandle;
}


VOID Process_Close( HANDLE ProcessHandle )
{
    NtClose(ProcessHandle);
}


VOID Process_EnumProcesses( ENUM_PROCESSES_CALLBACK Callback )
{
    UINT_B ProcThrdInfoSize = 0;
    UINT32 ProcOffset = 0;
    VOID *ProcThrdInfo = 0;
    LONG status = 0;
    SYSTEM_PROCESS_INFORMATION *processInfo;

    for (;;)
    {
        ProcThrdInfoSize += 0x10000;

        NtAllocateVirtualMemory((VOID*)-1,
            &ProcThrdInfo,
            0,
            &ProcThrdInfoSize,
            0x1000,   //MEM_COMMIT
            0x04);    //PAGE_READWRITE

        status = NtQuerySystemInformation(SystemProcessInformation,
            ProcThrdInfo,
            (UINT32)ProcThrdInfoSize,
            NULL);

        if (status == 0xC0000004)
        {
            NtFreeVirtualMemory((VOID*)-1,
                &ProcThrdInfo,
                (UINT32*)&ProcThrdInfoSize,
                0x8000); //MEM_RELEASE
            ProcThrdInfo = NULL;
        }
        else
        {

            break;
        }
    }

    processInfo = (SYSTEM_PROCESS_INFORMATION*)ProcThrdInfo;

    do
    {
        processInfo = (SYSTEM_PROCESS_INFORMATION*)((UINT_B)processInfo + ProcOffset);

        Callback(processInfo);

        ProcOffset = processInfo->NextEntryOffset;

    } while (ProcOffset != 0);

    return;
}


UINT32 Process_GetId( WCHAR* ProcessName, DWORD Ignore )
{
    UINT_B ProcThrdInfoSize = 0;
    UINT32 ProcOffset = 0;
    VOID *ProcThrdInfo = 0;
    LONG status = 0;
    UNICODE_STRING imageName;
    SYSTEM_PROCESS_INFORMATION *processInfo;

    for(;;)
    {
      ProcThrdInfoSize += 0x10000;

      NtAllocateVirtualMemory((VOID*)-1,
                              &ProcThrdInfo,
                              0,
                              &ProcThrdInfoSize,
                              0x1000,   //MEM_COMMIT
                              0x04);    //PAGE_READWRITE

      status = NtQuerySystemInformation(SystemProcessInformation,
                                        ProcThrdInfo,
                                        (UINT32) ProcThrdInfoSize,
                                        NULL);

      if (status == 0xC0000004)
      {
        NtFreeVirtualMemory((VOID*)-1,
                            &ProcThrdInfo,
                            (UINT32*)&ProcThrdInfoSize,
                            0x8000); //MEM_RELEASE
        ProcThrdInfo = NULL;
      }
      else
      {

        break;
      }
    }

    processInfo = (SYSTEM_PROCESS_INFORMATION*)ProcThrdInfo;

    RtlInitUnicodeString(&imageName, ProcessName);

    do
    {
      processInfo = (SYSTEM_PROCESS_INFORMATION*)((UINT_B)processInfo + ProcOffset);

      if (RtlCompareUnicodeString(&processInfo->ImageName, &imageName, FALSE) == 0 
          && (DWORD) processInfo->UniqueProcessId != Ignore)
      {
          return (UINT16) processInfo->UniqueProcessId;
      }

      ProcOffset = processInfo->NextEntryOffset;

    } while(ProcOffset != 0);

    return 0;
}


BOOLEAN Process_ThreadExists( UINT32 ProcessId, UINT32 ThreadId )
{
    UINT_B ProcThrdInfoSize = 0;
    UINT32 ProcOffset = 0;
    VOID *ProcThrdInfo = 0;
    LONG status = 0;
    SYSTEM_PROCESS_INFORMATION *processInfo;
    SYSTEM_THREAD_INFORMATION *threadInfo;

    for(;;)
    {
      ProcThrdInfoSize += 0x10000;

      NtAllocateVirtualMemory((VOID*)-1,
                              &ProcThrdInfo,
                              0,
                              &ProcThrdInfoSize,
                              0x1000,   //MEM_COMMIT
                              0x04);    //PAGE_READWRITE

      status = NtQuerySystemInformation(SystemProcessInformation,
                                        ProcThrdInfo,
                                        (UINT32) ProcThrdInfoSize,
                                        NULL);

      if (status == 0xC0000004)
      {
        NtFreeVirtualMemory((VOID*)-1,
                            &ProcThrdInfo,
                            (UINT32*)&ProcThrdInfoSize,
                            0x8000); //MEM_RELEASE
        ProcThrdInfo = NULL;
      }
      else
      {

        break;
      }
    }

    processInfo = (SYSTEM_PROCESS_INFORMATION*)ProcThrdInfo;

    do
    {
      processInfo = (SYSTEM_PROCESS_INFORMATION*)((UINT_B)processInfo + ProcOffset);

      if ((UINT32)processInfo->UniqueProcessId == ProcessId)
      {
          unsigned __int32 i;

        threadInfo = (SYSTEM_THREAD_INFORMATION*)(processInfo + 1);

        for(i = 0; i < processInfo->NumberOfThreads; i++)
        {
            if ((UINT32)threadInfo->ClientId.UniqueThread == ThreadId)
                return TRUE;

            threadInfo++;
        }
      }

      ProcOffset = processInfo->NextEntryOffset;

    } while(ProcOffset != 0);

    return FALSE;
}


NTSTATUS Process_GetFileNameByHandle( HANDLE ProcessHandle, WCHAR* FileName )
{
    UINT16 bufferSize;
    UNICODE_STRING *imageFileName;
    NTSTATUS status;

    bufferSize = sizeof(UNICODE_STRING) + (260 * sizeof(WCHAR));
    imageFileName = (UNICODE_STRING*)Memory_Allocate(bufferSize);
    status = NtQueryInformationProcess(ProcessHandle, ProcessImageFileNameWin32, imageFileName, bufferSize, NULL);

    Memory_Copy(FileName, imageFileName->Buffer, imageFileName->Length);

    FileName[imageFileName->Length / sizeof(WCHAR)] = L'\0';

    return status;
}


// GetModuleFileNameW is faster
BOOLEAN Process_GetFileNameByProcessId( UINT32 processID, WCHAR* buffer )
{
    VOID *processHandle = 0;

    processHandle = Process_Open(processID, PROCESS_QUERY_INFORMATION);

    Process_GetFileNameByHandle(processHandle, buffer);

    if (processHandle)
        return TRUE;
    else
        return FALSE;
}





#define PAGE_WRITECOPY   0x08
#define PAGE_EXECUTE_WRITECOPY   0x80


VOID Process_WriteMemory( HANDLE ProcessHandle, VOID* BaseAddress, VOID* Buffer, SIZE_T Size )
{
    VOID *baseAddress;
    SIZE_T regionSize;
    SIZE_T bytesWritten;
    ULONG oldValue;
    BOOLEAN unprotect;

    baseAddress = BaseAddress;
    regionSize = Size;

    NtProtectVirtualMemory(ProcessHandle, &baseAddress, &regionSize, PAGE_EXECUTE_READWRITE, &oldValue);

    /* Check if we are unprotecting */
    unprotect = oldValue & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY) ? FALSE : TRUE;

    if (!unprotect)
    {
        /* Set the new protection */
        NtProtectVirtualMemory(ProcessHandle, &baseAddress, &regionSize, oldValue, &oldValue);

        /* Write the memory */
        NtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, &bytesWritten);
    }
    else
    {
        /* do the write */
        NtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, &bytesWritten);

        /* And restore the protection */
        NtProtectVirtualMemory(ProcessHandle, &baseAddress, &regionSize, oldValue, &oldValue);
    }

    /* Flush the ITLB */
    //NtFlushInstructionCache(ProcessHandle, baseAddress, sizeof(Buffer));
}


long __stdcall NtSuspendProcess( VOID* hProcessHandle );
long __stdcall NtResumeProcess( VOID* hProcessHandle );


VOID Process_Suspend( HANDLE ProcessHandle )
{
    NtSuspendProcess( ProcessHandle );
}


VOID Process_Resume( HANDLE ProcessHandle )
{
    NtResumeProcess( ProcessHandle );
}


BOOLEAN Process_IsWow64( HANDLE ProcessHandle )
{
    INTBOOL isWow64;

    NtQueryInformationProcess(ProcessHandle, ProcessWow64Information, &isWow64, sizeof(INTBOOL), NULL);

    if (isWow64)
        return TRUE;
    else
        return FALSE;
}
