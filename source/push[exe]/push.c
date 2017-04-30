#include <sl.h>
#include <slresource.h>
#include <slregistry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pushbase.h>
#include <gui.h>
#include <osd.h>
#include <slprocess.h>
#include <sldebug.h>

#include "push.h"
#include "ramdisk.h"
#include "Hardware\hardware.h"
#include "batch.h"
#include "ring0.h"
#include "file.h"
#include "driver.h"


WCHAR g_szPrevGame[260];
WCHAR g_szLastDir[260];
BOOLEAN g_bRecache;
VOID* PushControlHandles[50];
VOID* PushInstance;
VOID* PushMonitorThreadHandle;
VOID* scmHandle;
VOID* R0DriverHandle    = INVALID_HANDLE_VALUE;
FILE_LIST PushFileList    = 0;
VOID* PushHeapHandle;
UINT32 thisPID;
PUSH_SHARED_MEMORY* PushSharedMemory;
OVERLAY_INTERFACE PushOverlayInterface = OVERLAY_INTERFACE_PURE;
extern SYSTEM_BASIC_INFORMATION HwInfoSystemBasicInformation;
UINT32 GameProcessId;


typedef int (__stdcall* TYPE_MuteJack)(CHAR *pin);
TYPE_MuteJack MuteJack;

DWORD __stdcall MapFileAndCheckSumW(
    WCHAR* Filename,
    DWORD* HeaderSum,
    DWORD* CheckSum
    );


BOOLEAN IsGame( WCHAR* ExecutablePath )
{
    WCHAR *ps;

    ps = SlIniReadString(L"Games", ExecutablePath, 0, L".\\" PUSH_SETTINGS_FILE);

    if (ps != 0)
    {
        //is game
        RtlFreeHeap(PushHeapHandle, 0, ps);

        return TRUE;
    }
    else
    {
        // Try searching for names that match. If a match is found, compare the executable's checksum.

        DWORD headerSum;
        DWORD checkSum;
        GAME_LIST gameList = Game_GetGames();
        wchar_t *executable = String_FindLastChar(ExecutablePath, '\\');

        executable++;

        MapFileAndCheckSumW(ExecutablePath, &headerSum, &checkSum);

        while (gameList != NULL)
        {
            if (String_Compare(gameList->Game->ExecutableName, executable) == 0)
            {
                if (gameList->Game->CheckSum == checkSum)
                {
                    // Update path.

                    SlIniWriteString(
                        L"Games",
                        gameList->Game->ExecutablePath,
                        NULL,
                        L".\\" PUSH_SETTINGS_FILE
                        );

                    SlIniWriteString(
                        L"Games",
                        ExecutablePath,
                        gameList->Game->Id,
                        L".\\" PUSH_SETTINGS_FILE
                        );

                    return TRUE;
                }
            }

            gameList = gameList->NextEntry;
        }
    }

    return FALSE;
}


INTBOOL __stdcall SetWindowTextW(
    VOID* hWnd,
    WCHAR* lpString
    );


VOID
CopyProgress(
    UINT64 TotalSize,
    UINT64 TotalTransferred
    )
{
    WCHAR progressText[260];

    swprintf(
        progressText,
        260,
        L"%I64d / %I64d",
        TotalTransferred,
        TotalSize
        );

    SetWindowTextW(
        CpwTextBoxHandle,
        progressText
        );
}


VOID
CacheFile( WCHAR *FileName, CHAR cMountPoint )
{
    WCHAR destination[260];
    WCHAR *pszFileName;
    CHAR bMarkedForCaching = FALSE;

    WCHAR* slash;
    WCHAR deviceName[260], dosName[260];

    destination[0] = cMountPoint;
    destination[1] = ':';
    destination[2] = '\\';
    destination[3] = '\0';

    pszFileName = String_FindLastChar(FileName, '\\') + 1;

    if (!bMarkedForCaching)
        // file was a member of a folder marked for caching
    {
        String_Concatenate(destination, g_szLastDir);
        String_Concatenate(destination, L"\\");
    }

    String_Concatenate(destination, pszFileName);
    File_Copy(FileName, destination, CopyProgress);
    String_Copy(dosName, FileName);

    slash = String_FindFirstChar(dosName, '\\');
    *slash = L'\0';

    QueryDosDeviceW(dosName, deviceName, 260);

    String_Concatenate(deviceName, L"\\");
    String_Concatenate(deviceName, slash + 1);

    R0QueueFile(
        deviceName,
        String_GetLength(deviceName) + 1
             );
}


NTSTATUS __stdcall NtTerminateThread(
    VOID* ThreadHandle,
    NTSTATUS ExitStatus
    );

#define MB_TOPMOST          0x00040000L


VOID
CacheFiles(CHAR driveLetter)
{
    FILE_LIST_ENTRY *file = (FILE_LIST_ENTRY*) PushFileList;
    VOID *threadHandle;

    //create copy progress window
    /*threadHandle = CreateThread(
                    NULL,
                    NULL,
                    &CpwThread,
                    NULL,
                    NULL,
                    NULL
                    );*/

    threadHandle = CreateRemoteThread(
                    NtCurrentProcess(),
                    NULL,
                    NULL,
                    &CpwThread,
                    NULL,
                    NULL,
                    NULL
                    );

    while (file != 0)
    {
        CacheFile(file->Name, driveLetter);

        file = file->NextEntry;
    }

    //close the window
    PostMessageW( CpwWindowHandle, WM_CLOSE, 0, 0 );

    //destroy the thread
    NtTerminateThread(threadHandle, 0);
}


VOID
PushAddToFileList( FILE_LIST* FileList, FILE_LIST_ENTRY *FileEntry )
{
    FILE_LIST_ENTRY *fileListEntry;
    WCHAR *name;

    name = (WCHAR*) RtlAllocateHeap(
            PushHeapHandle,
            0,
            (String_GetLength(FileEntry->Name) + 1) * sizeof(WCHAR)
            );

    String_Copy(name, FileEntry->Name);

    if (*FileList == NULL)
    {
        FILE_LIST_ENTRY *fileList;

        *FileList = (FILE_LIST) RtlAllocateHeap(
            PushHeapHandle,
            0,
            sizeof(FILE_LIST_ENTRY)
            );

        fileList = *FileList;

        fileList->NextEntry = NULL;
        fileList->Bytes = FileEntry->Bytes;
        fileList->Name = name;
        fileList->Cache = FileEntry->Cache;

        return;
    }

    fileListEntry = (FILE_LIST_ENTRY*) *FileList;

    while (fileListEntry->NextEntry != 0)
    {
        fileListEntry = fileListEntry->NextEntry;
    }

    fileListEntry->NextEntry = (FILE_LIST_ENTRY *) RtlAllocateHeap(
        PushHeapHandle,
        0,
        sizeof(FILE_LIST_ENTRY)
        );

    fileListEntry = fileListEntry->NextEntry;

    fileListEntry->Bytes = FileEntry->Bytes;
    fileListEntry->Name = name;
    fileListEntry->Cache = FileEntry->Cache;
    fileListEntry->NextEntry = 0;
}


VOID Cache( PUSH_GAME* Game )
{
    CHAR mountPoint = 0;
    UINT64 bytes = 0, availableMemory = 0; // in bytes
    SYSTEM_BASIC_PERFORMANCE_INFORMATION performanceInfo;
    //BfBatchFile batchFile(Game);

    // Check if game is already cached so we donot have wait through another
    if (String_Compare(g_szPrevGame, Game->InstallPath) == 0 && !g_bRecache)
        return;

    g_bRecache = FALSE;

    if (!FolderExists(Game->InstallPath))
    {
        //MessageBoxW(0, L"Folder not exist!", 0, MB_TOPMOST);
        OutputDebugStringW(L"Folder not exist!");
    }

    // Check available memory
    NtQuerySystemInformation(
        SystemBasicPerformanceInformation,
        &performanceInfo,
        sizeof(SYSTEM_BASIC_PERFORMANCE_INFORMATION),
        NULL
        );

    availableMemory = performanceInfo.AvailablePages * HwInfoSystemBasicInformation.PageSize;

    // Read batch file
    PushFileList = 0;

    BatchFile_Initialize(Game);
    bytes = BatchFile_GetBatchSize();
    PushFileList = BatchFile_GetBatchList();

    // Check if any files at all are marked for cache, if not return.
    if (PushFileList == NULL)
        // List is empty hence no files to cache, return.
        return;

    // Add 200MB padding for disk format;
    bytes += 209715200;

    if (bytes > availableMemory)
    {
        MessageBoxW(
            NULL,
            L"There isn't enough memory to hold all the files you marked for caching.\n"
            L"The Ramdisk will be set to an acceptable size and filled with what it can hold.\n"
            L"Please upgrade RAM.",
            L"Push",
            MB_TOPMOST
            );
    }

    RemoveRamDisk();

    mountPoint = FindFreeDriveLetter();

    CreateRamDisk(bytes, mountPoint);
    FormatRamDisk();
    CacheFiles(mountPoint);

    // Release batchfile list
    PushFileList = NULL;

    String_Copy(g_szPrevGame, Game->InstallPath);
}


VOID OnProcessEvent( PROCESSID processID )
{
    WCHAR fileName[260];
    VOID *processHandle = NULL;
    UINT32 iBytesRead;
    WCHAR *result = 0;
    CHAR szCommand[] = "MUTEPIN 1 a";

    processHandle = Process_Open(processID, PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME);

    if (!processHandle)
        return;

    Process_GetFileNameByHandle(processHandle, fileName);

    if (IsGame(fileName))
    {
        PUSH_GAME game = { 0 };

        Game_Initialize(fileName, &game);

        if (game.Settings.UseRamDisk)
        {
            PushSharedMemory->GameUsesRamDisk = TRUE;

            //suspend process to allow us time to cache files
            Process_Suspend(processHandle);
            Cache(&game);
        }

        PushSharedMemory->DisableRepeatKeys = game.Settings.DisableRepeatKeys;
        PushSharedMemory->SwapWASD = game.Settings.SwapWASD;
        OutputDebugStringW(L"SETTING SHARED MEM!!!!!");
        PushSharedMemory->VsyncOverrideMode = game.Settings.VsyncOverrideMode;

        // Check if user wants maximum gpu engine and memory clocks
        if (game.Settings.ForceMaxClocks)
        {
            Hardware_ForceMaxClocks();
        }

        // i used this to disable one of my audio ports while gaming but of course it probably only
        // works for IDT audio devices
        CallNamedPipeW(
            L"\\\\.\\pipe\\stacsv",
            szCommand,
            sizeof(szCommand),
            0,
            0,
            &iBytesRead,
            NMPWAIT_WAIT_FOREVER
            );

        if (PushSharedMemory->GameUsesRamDisk)
            //resume process
            Process_Resume(processHandle);
    }
    else
    {
        PushSharedMemory->GameUsesRamDisk = FALSE;
    }

    Process_Close(processHandle);

    PushSharedMemory->OSDFlags |= OSD_FPS; //enable fps counter
}


VOID Inject32( VOID *hProcess )
{
    VOID *threadHandle, *pLibRemote, *hKernel32;
    WCHAR szModulePath[260], *pszLastSlash;

    hKernel32 = GetModuleHandleW(L"Kernel32");

    GetModuleFileNameW(0, szModulePath, 260);

    pszLastSlash = String_FindLastChar(szModulePath, '\\');

    pszLastSlash[1] = '\0';

    String_Concatenate(szModulePath, PUSH_LIB_NAME_32);

    // Allocate remote memory
    pLibRemote = VirtualAllocEx(hProcess, 0, sizeof(szModulePath), MEM_COMMIT, PAGE_READWRITE);

    // Copy library name
    Process_WriteMemory(hProcess, pLibRemote, szModulePath, sizeof(szModulePath));

    // Load dll into the remote process
    threadHandle = CreateRemoteThread(hProcess,
                                 0,0,
                                 (PTHREAD_START_ROUTINE) Module_GetProcedureAddress(hKernel32, "LoadLibraryW"),
                                 pLibRemote,
                                 0,0);

    WaitForSingleObject(threadHandle, INFINITE );

    // Clean up
    //CloseHandle(hThread);
    NtClose(threadHandle);

    VirtualFreeEx(hProcess, pLibRemote, sizeof(szModulePath), MEM_RELEASE);
}


typedef struct _SECURITY_DESCRIPTOR {
    UCHAR Revision;
    UCHAR Sbz1;
    WORD Control;
    VOID* Owner;
    VOID* Group;
    VOID* Sacl;
    VOID* Dacl;
} SECURITY_DESCRIPTOR;

#define DACL_SECURITY_INFORMATION               (0x00000004L)
#define UNPROTECTED_DACL_SECURITY_INFORMATION   (0x20000000L)
#define FILE_END             2

NTSTATUS __stdcall NtQuerySecurityObject(
    HANDLE Handle,
    DWORD SecurityInformation,
    SECURITY_DESCRIPTOR* SecurityDescriptor,
    ULONG Length,
    ULONG* LengthNeeded
    );

NTSTATUS __stdcall NtSetSecurityObject(
    HANDLE Handle,
    ULONG SecurityInformation,
    VOID* SecurityDescriptor
    );

DWORD __stdcall SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    LONG* lpDistanceToMoveHigh,
    DWORD dwMoveMethod
    );

VOID Inject64(UINT32 ProcessId, WCHAR* Path);


VOID OnImageEvent( PROCESSID ProcessId )
{
    VOID *processHandle = 0;
    wchar_t filePath[260];
    wchar_t *executableName;
    NTSTATUS status;

    processHandle = Process_Open(
        ProcessId,
        PROCESS_VM_OPERATION |
        PROCESS_VM_READ |
        PROCESS_VM_WRITE |
        PROCESS_VM_OPERATION |
        PROCESS_CREATE_THREAD |
        PROCESS_QUERY_INFORMATION |
        SYNCHRONIZE
        );

    if (!processHandle)
    {
        NTSTATUS status;
        ULONG bufferSize;
        SECURITY_DESCRIPTOR* securityDescriptor;

        bufferSize = 0x100;
        securityDescriptor = (SECURITY_DESCRIPTOR*)RtlAllocateHeap(PushHeapHandle, 0, bufferSize);

        // Get the DACL of this process since we know we have all rights in it.
        status = NtQuerySecurityObject(
            NtCurrentProcess(),
            DACL_SECURITY_INFORMATION,
            securityDescriptor,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            Memory_Free(securityDescriptor);
            securityDescriptor = (SECURITY_DESCRIPTOR*)RtlAllocateHeap(PushHeapHandle, 0, bufferSize);

            status = NtQuerySecurityObject(
                NtCurrentProcess(),
                DACL_SECURITY_INFORMATION,
                securityDescriptor,
                bufferSize,
                &bufferSize
                );
        }

        if (!NT_SUCCESS(status))
        {
            Memory_Free(securityDescriptor);
            return;
        }

        // Open it with WRITE_DAC access so that we can write to the DACL.
        processHandle = Process_Open(ProcessId, (0x00040000L)); //WRITE_DAC

        if(processHandle == 0)
        {
            Memory_Free(securityDescriptor);
           return;
        }

        status = NtSetSecurityObject(
            processHandle,
            DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
            securityDescriptor
            );

        if (!NT_SUCCESS(status))
        {
            Memory_Free(securityDescriptor);
            return;
        }

        // The DACL is overwritten with our own DACL. We
        // should be able to open it with the requested
        // privileges now.

        Process_Close(processHandle);

        processHandle = 0;
        Memory_Free(securityDescriptor);

        processHandle = Process_Open(
            ProcessId,
            PROCESS_VM_OPERATION |
            PROCESS_VM_READ |
            PROCESS_VM_WRITE |
            PROCESS_VM_OPERATION |
            PROCESS_CREATE_THREAD |
            PROCESS_QUERY_INFORMATION |
            SYNCHRONIZE
            );
    }

    if (!processHandle)
    {
        return;
    }
    
    status = Process_GetFileNameByHandle(processHandle, filePath);

    if (IsGame(filePath))
    {
        PUSH_GAME game = { 0 };

        Game_Initialize(filePath, &game);

        if (game.Settings.DisableOverlay)
        {
            Log(L"Skipping injection on %s", game.ExecutableName);

            return;
        }
    }

    if (NT_SUCCESS(status))
    {
        executableName = String_FindLastChar(filePath, '\\');
        executableName++;

        Log(L"Injecting into %s", executableName);
    }

    GameProcessId = ProcessId;

    if (Process_IsWow64(processHandle))
    {
        if (PushOverlayInterface == OVERLAY_INTERFACE_PURE)
        {
            Resource_Extract(L"OVERLAY32", PUSH_LIB_NAME_32);
            Inject32(processHandle);
        }
        else
        {
            if (!Process_GetId(L"RTSS.exe", 0))
            {
                MessageBoxW(0, L"Rivatuner Statistics Server not running!", 0, 0);
            }
        }
    }
    else
    {
        if (PushOverlayInterface == OVERLAY_INTERFACE_PURE)
        {
            WCHAR szModulePath[260], *pszLastSlash;

            GetModuleFileNameW(0, szModulePath, 260);

            pszLastSlash = String_FindLastChar(szModulePath, '\\');
            pszLastSlash[1] = '\0';

            Resource_Extract(L"OVERLAY64", PUSH_LIB_NAME_64);
            String_Concatenate(szModulePath, PUSH_LIB_NAME_64);
            
            #ifdef _MSC_PLATFORM_TOOLSET_v120
            Inject64(ProcessId, szModulePath);
            #endif
        }
    }

    Process_Close(processHandle);
}


DWORD __stdcall RetrieveProcessEvent( VOID* Parameter )
{
    OVERLAPPED              ov          = { 0 };
    //BOOLEAN                    bReturnCode = FALSE;
    UINT32                  iBytesReturned;
    PROCESS_CALLBACK_INFO   processInfo;


    // Create an event handle for async notification from the driver

    ov.hEvent = CreateEventW(
        0,  // Default security
        TRUE,  // Manual reset
        FALSE, // non-signaled state
        0
        );

    // Get the process info
    PushGetProcessInfo(&processInfo);

    //
    // Wait here for the event handle to be set, indicating
    // that the IOCTL processing is completed.
    //
    GetOverlappedResult(
        R0DriverHandle,
        &ov,
        &iBytesReturned,
        TRUE
        );

    OnProcessEvent(processInfo.hProcessID);
    NtClose(ov.hEvent);

    return 0;
}


DWORD __stdcall RetrieveImageEvent( VOID* Parameter )
{
    OVERLAPPED          ov          = { 0 };
    //BOOLEAN                bReturnCode = FALSE;
    UINT32              iBytesReturned;
    IMAGE_CALLBACK_INFO imageInfo;

    // Create an event handle for async notification from the driver
    ov.hEvent = CreateEventW(
        NULL,  // Default security
        TRUE,  // Manual reset
        FALSE, // non-signaled state
        NULL
        );

    // Get the process info
    PushGetImageInfo(&imageInfo);

    Log(L"%i loaded D3D module", imageInfo.processID);


    //
    // Wait here for the event handle to be set, indicating
    // that the IOCTL processing is completed.
    //
    GetOverlappedResult(
        R0DriverHandle,
        &ov,
        &iBytesReturned,
        TRUE
        );

    OnImageEvent(imageInfo.processID);
    NtClose(ov.hEvent);

    return 0;
}


DWORD __stdcall MonitorThread(VOID* Parameter)
{
    VOID *processEvent, *d3dImageEvent;
    VOID *handles[2];

    processEvent = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\" PUSH_PROCESS_EVENT_NAME);
    d3dImageEvent = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\" PUSH_IMAGE_EVENT_NAME);

    handles[0] = processEvent;
    handles[1] = d3dImageEvent;

    while (processEvent)
    {
        DWORD result = 0;

        result = WaitForMultipleObjectsEx(
            sizeof(handles) / sizeof(handles[0]),
            &handles[0],
            FALSE,
            INFINITE,
            FALSE
            );

        if (processEvent && handles[result - WAIT_OBJECT_0] == processEvent)
        {
            CreateRemoteThread(NtCurrentProcess(), 0, 0, &RetrieveProcessEvent, NULL, 0, NULL);
        }
        else if (handles[result - WAIT_OBJECT_0] == d3dImageEvent)
        {
            CreateRemoteThread(NtCurrentProcess(), 0, 0, &RetrieveImageEvent, NULL, 0, NULL);
        }
    }

    return 0;
}


#define PIPE_ACCESS_DUPLEX          0x00000003
#define PIPE_TYPE_BYTE              0x00000000
#define PIPE_READMODE_BYTE          0x00000000
#define PIPE_WAIT                   0x00000000
#define NMPWAIT_USE_DEFAULT_WAIT        0x00000000


INTBOOL __stdcall ConnectNamedPipe(
        HANDLE hNamedPipe,
        OVERLAPPED* lpOverlapped
        );

    INTBOOL __stdcall DisconnectNamedPipe(
        HANDLE hNamedPipe
        );

    NTSTATUS __stdcall NtCreateNamedPipeFile(
        _Out_ HANDLE* FileHandle,
        _In_ ULONG DesiredAccess,
        _In_ OBJECT_ATTRIBUTES* ObjectAttributes,
        _Out_ PIO_STATUS_BLOCK IoStatusBlock,
        _In_ ULONG ShareAccess,
        _In_ ULONG CreateDisposition,
        _In_ ULONG CreateOptions,
        _In_ ULONG NamedPipeType,
        _In_ ULONG ReadMode,
        _In_ ULONG CompletionMode,
        _In_ ULONG MaximumInstances,
        _In_ ULONG InboundQuota,
        _In_ ULONG OutboundQuota,
        _In_opt_ LARGE_INTEGER* DefaultTimeout
        );


#include "Hardware\GPU\adl.h"
#define PIPE_ACCEPT_REMOTE_CLIENTS 0x00000000
    VOID Log(const wchar_t* Format, ...)
    {
        wchar_t buffer[260];
        wchar_t output[260] = L"[PUSH] ";
        va_list _Arglist;

        _crt_va_start(_Arglist, Format);
        _vswprintf_c_l(buffer, 260, Format, NULL, _Arglist);
        _crt_va_end(_Arglist);

        wcsncat(output, buffer, 260);
        OutputDebugStringW(output);
    }

DWORD __stdcall PipeThread( VOID* Parameter )
{
    HANDLE pipeHandle;
    WCHAR buffer[1024];
    OBJECT_ATTRIBUTES objAttributes;
    UNICODE_STRING pipeName;
    IO_STATUS_BLOCK isb;
    LARGE_INTEGER timeOut;

    /*pipeHandle = CreateNamedPipeW(
        L"\\\\.\\pipe\\Push", 
        PIPE_ACCESS_DUPLEX, 
        PIPE_WAIT | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_ACCEPT_REMOTE_CLIENTS, 
        1, 
        1024 * 16,
        1024 * 16,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
        );*/

    RtlDosPathNameToNtPathName_U(L"\\\\.\\pipe\\Push", &pipeName, NULL, NULL);

    objAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttributes.RootDirectory = NULL;
    objAttributes.ObjectName = &pipeName;
    objAttributes.Attributes = OBJ_CASE_INSENSITIVE;
    objAttributes.SecurityDescriptor = NULL;
    objAttributes.SecurityQualityOfService = NULL;

    timeOut.QuadPart = 0xfffffffffff85ee0;

    NtCreateNamedPipeFile(
        &pipeHandle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        &objAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        0,
        0,
        0,
        1,
        1024 * 16,
        1024 * 16,
        &timeOut
        );

    while (pipeHandle != INVALID_HANDLE_VALUE)
    {
        if (ConnectNamedPipe(pipeHandle, NULL) != FALSE)   // wait for someone to connect to the pipe
        {
            IO_STATUS_BLOCK isb;

            while (NtReadFile(
                pipeHandle, 
                NULL, 
                NULL, 
                NULL, 
                &isb, 
                buffer, 
                sizeof(buffer) - 1, 
                NULL, 
                NULL
                ) == STATUS_SUCCESS)
            {
                /* add terminating zero */
                buffer[isb.Information] = '\0';

                if (String_Compare(buffer, L"ForceMaxClocks") == 0)
                {
                    Hardware_ForceMaxClocks();
                } 
                else if (String_CompareN(buffer, L"Overclock", 8) == 0)
                {
                    switch (buffer[10])
                    {
                    case 'e':
                        {
                            switch (buffer[12])
                            {
                            case 'i':
                                Adl_SetEngineClock(PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock + 1, 2);
                                break;
                            case 'd':
                                Adl_SetEngineClock(PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock - 1, 2);
                                break;
                            }
                        }
                        break;
                    case 'm':
                        Adl_SetMemoryClock(PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClock + 1, 2);
                        break;
                    case 'v':
                        Adl_SetVoltage(PushSharedMemory->HarwareInformation.DisplayDevice.Voltage + 1);
                        break;
                    }
                }
                else if (String_Compare(buffer, L"UpdateClocks") == 0)
                {
                    Adl_SetEngineClock(PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax, 2);
                    Adl_SetMemoryClock(PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax, 2);
                    Adl_SetVoltage(PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax);

                    PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax = Adl_GetEngineClockMax();
                }
                else if (String_CompareN(buffer, L"GetDiskResponseTime", 19) == 0)
                {
                    UINT32 processId;
                    UINT16 responseTime;

                    processId = String_ToInteger(&buffer[20]);
                    responseTime = GetDiskResponseTime(processId);

                    File_Write(pipeHandle, &responseTime, 2);
                }
            }
        }

        DisconnectNamedPipe(pipeHandle);
    }

    return 0;
}


INT32 __stdcall WinMain( VOID* Instance, VOID *hPrevInstance, CHAR *pszCmdLine, INT32 iCmdShow )
{
    VOID *sectionHandle = INVALID_HANDLE_VALUE, *hMutex;
    MSG messages;
    BOOLEAN bAlreadyRunning;
    OBJECT_ATTRIBUTES objAttrib = {0};

    // Check if already running
    hMutex = CreateMutexW(0, FALSE, L"PushOneInstance");

    bAlreadyRunning = (GetLastError() == ERROR_ALREADY_EXISTS
                        || GetLastError() == ERROR_ACCESS_DENIED);

    if (bAlreadyRunning)
    {
        MessageBoxW(0, L"Only one instance!", 0,0);
        ExitProcess(0);
    }

    thisPID = (UINT32) NtCurrentTeb()->ClientId.UniqueProcess;
    PushHeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    //create image event
    CreateEventW(NULL, TRUE, FALSE, L"Global\\" PUSH_IMAGE_EVENT_NAME);

    // Start Driver.

    Driver_Extract();
    Driver_Load();

    //initialize instance
    PushInstance = Instance;

    // Create interface
    MwCreateMainWindow();

    // Create file mapping.
    PushSharedMemory = (PUSH_SHARED_MEMORY*)Memory_CreateFileMapping(PUSH_SECTION_NAME, sizeof(PUSH_SHARED_MEMORY));

    if (!PushSharedMemory)
    {
        OutputDebugStringW(L"Could not create shared memory");
        return 0;
    }

    //zero struct
    memset(PushSharedMemory, 0, sizeof(PUSH_SHARED_MEMORY));

    //initialize window handle used by overlay
    PushSharedMemory->WindowHandle = PushMainWindow->Handle;

    if (File_Exists(PUSH_SETTINGS_FILE))
    {
        WCHAR *buffer;

        // Check if file is UTF-16LE.
        buffer = (WCHAR*) File_Load(PUSH_SETTINGS_FILE, NULL);

        if (buffer[0] == 0xFEFF)
            //is UTF-LE.
        {
            // Init settings from ini file.

            if (SlIniReadBoolean(L"Settings", L"FrameLimit", FALSE, L".\\" PUSH_SETTINGS_FILE))
                PushSharedMemory->FrameLimit = TRUE;

            if (SlIniReadBoolean(L"Settings", L"ThreadOptimization", FALSE, L".\\" PUSH_SETTINGS_FILE))
                PushSharedMemory->ThreadOptimization = TRUE;

            if (SlIniReadBoolean(L"Settings", L"KeepFps", FALSE, L".\\" PUSH_SETTINGS_FILE))
                PushSharedMemory->KeepFps = TRUE;

            buffer = SlIniReadString(L"Settings", L"OverlayInterface", NULL, L".\\" PUSH_SETTINGS_FILE);

            if (String_Compare(buffer, L"PURE") == 0)
                PushOverlayInterface = OVERLAY_INTERFACE_PURE;
            else if (String_Compare(buffer, L"RTSS") == 0)
                PushOverlayInterface = OVERLAY_INTERFACE_RTSS;

            buffer = SlIniReadString(L"Settings", L"KeyboardHookType", L"AUTO", L".\\" PUSH_SETTINGS_FILE);

            if (String_Compare(buffer, L"AUTO") == 0)
            {
                PushSharedMemory->KeyboardHookType = KEYBOARD_HOOK_AUTO;
            }
            else if (String_Compare(buffer, L"SUBCLASS") == 0)
            {
                PushSharedMemory->KeyboardHookType = KEYBOARD_HOOK_SUBCLASS;
            }
            else if (String_Compare(buffer, L"MESSAGE") == 0)
            {
                PushSharedMemory->KeyboardHookType = KEYBOARD_HOOK_MESSAGE;
            }
            else if (String_Compare(buffer, L"KEYBOARD") == 0)
            {
                PushSharedMemory->KeyboardHookType = KEYBOARD_HOOK_KEYBOARD;
            }
            else if (String_Compare(buffer, L"DETOURS") == 0)
            {
                PushSharedMemory->KeyboardHookType = KEYBOARD_HOOK_DETOURS;
            }
            else if (String_Compare(buffer, L"RAW") == 0)
            {
                PushSharedMemory->KeyboardHookType = KEYBOARD_HOOK_RAW;
            }
            else
            {
                PushSharedMemory->KeyboardHookType = KEYBOARD_HOOK_AUTO;
            }

            buffer = SlIniReadString(L"Settings", L"EngineClockMax", NULL, L".\\" PUSH_SETTINGS_FILE);
            PushSharedMemory->HarwareInformation.DisplayDevice.EngineOverclock = _wtoi(buffer);

            buffer = SlIniReadString(L"Settings", L"MemoryClockMax", NULL, L".\\" PUSH_SETTINGS_FILE);
            PushSharedMemory->HarwareInformation.DisplayDevice.MemoryOverclock = _wtoi(buffer);
        }
        else
        {
            MessageBoxW(
                NULL,
                L"Settings file not UTF-16LE! "
                L"Resave the file as \"Unicode\" or Push won't read it!",
                L"Bad Settings file",
                NULL
                );
        }
    }

    //initialize HWInfo
    GetHardwareInfo();

    //initialize OSD items
    OSD_Initialize();

    //start timer
    SetTimer(PushMainWindow->Handle, 0, 1000, 0);

    // Activate process monitoring

    PushToggleProcessMonitoring(TRUE);

    g_szPrevGame[5] = '\0';

    PushMonitorThreadHandle = CreateRemoteThread(NtCurrentProcess(), 0, 0, &MonitorThread, NULL, 0, NULL);

    CreateRemoteThread(NtCurrentProcess(), NULL, 0, &PipeThread, NULL, 0, NULL);

    // Handle messages

    while(GetMessageW(&messages, 0,0,0))
    {
        TranslateMessage(&messages);

        DispatchMessageW(&messages);
    }

    return 0;
}


WCHAR* GetDirectoryFile( WCHAR *pszFileName )
{
    static WCHAR szPath[260];

    GetCurrentDirectoryW(260, szPath);

    String_Concatenate(szPath, L"\\");
    String_Concatenate(szPath, pszFileName);

    return szPath;
}


/*VOID
UpdateSharedMemory()
{
    PushSharedMemory->HarwareInformation = hardware;
}*/


VOID PushOnTimer()
{
    RefreshHardwareInfo();
    //UpdateSharedMemory();
    OSD_Refresh();
}


VOID
GetTime(CHAR *pszBuffer)
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );

    timeinfo = localtime ( &rawtime );

    strftime (pszBuffer,80,"%H:%M:%S",timeinfo);
}


VOID Push_FormatTime( WCHAR* Buffer )
{
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);

    timeinfo = localtime(
        &rawtime
        );

    wcsftime(
        Buffer,
        20,
        L"%H:%M:%S",
        timeinfo
        );
}



