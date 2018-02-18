#include <push.h>


WCHAR g_szPrevGame[260];
WCHAR g_szLastDir[260];
WCHAR PushFilePath[260];
BOOLEAN g_bRecache;
VOID* PushControlHandles[50];
VOID* PushInstance;
VOID* PushMonitorThreadHandle;
VOID* scmHandle;
VOID* R0DriverHandle    = INVALID_HANDLE_VALUE;
FILE_LIST PushFileList    = 0;
VOID* PushHeapHandle;
UINT32 PushProcessId;
ULONG PushSessionId;
PUSH_SHARED_MEMORY* PushSharedMemory;
OVERLAY_INTERFACE PushOverlayInterface = OVERLAY_INTERFACE_PURE;
extern SYSTEM_BASIC_INFORMATION HwInfoSystemBasicInformation;
UINT32 GameProcessId;


typedef int (__stdcall* TYPE_MuteJack)(CHAR *pin);
TYPE_MuteJack MuteJack;


VOID InitializeCRT();


INTBOOL __stdcall SetWindowTextW(
    VOID* hWnd,
    WCHAR* lpString
    );


VOID CopyProgress( UINT64 TotalSize, UINT64 TotalTransferred )
{
    WCHAR progressText[260];

    String_Format(progressText, 260, L"%I64d / %I64d", TotalTransferred, TotalSize);
    SetWindowTextW(CpwTextBoxHandle, progressText);
}


NTSTATUS __stdcall NtOpenDirectoryObject(
    HANDLE* DirectoryHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes
    );

NTSTATUS __stdcall NtOpenSymbolicLinkObject(
    HANDLE* LinkHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes
    );

NTSTATUS __stdcall NtQuerySymbolicLinkObject(
    HANDLE LinkHandle,
    UNICODE_STRING* LinkTarget,
    ULONG* ReturnedLength
    );

#define SYMBOLIC_LINK_QUERY 0x0001
#define DIRECTORY_QUERY 0x0001


VOID CacheFile( WCHAR *FileName, CHAR cMountPoint )
{
    WCHAR destination[260];
    WCHAR *pszFileName;
    CHAR bMarkedForCaching = FALSE;
    WCHAR* slash;
    WCHAR deviceName[260], dosName[260];
    HANDLE directoryHandle;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objAttrib;
    UNICODE_STRING directoryName;
    UNICODE_STRING driveLetter;
    UNICODE_STRING linkTarget;

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

    UnicodeString_Init(&directoryName, L"\\??");

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = NULL;
    objAttrib.ObjectName = &directoryName;
    objAttrib.Attributes = OBJ_CASE_INSENSITIVE;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    NtOpenDirectoryObject(&directoryHandle, DIRECTORY_QUERY, &objAttrib);

    UnicodeString_Init(&driveLetter, dosName);

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = directoryHandle;
    objAttrib.ObjectName = &driveLetter;
    objAttrib.Attributes = OBJ_CASE_INSENSITIVE;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    NtOpenSymbolicLinkObject(&linkHandle, SYMBOLIC_LINK_QUERY, &objAttrib);

    linkTarget.Length = 0;
    linkTarget.MaximumLength = 260 * sizeof(WCHAR);
    linkTarget.Buffer = deviceName;

    NtQuerySymbolicLinkObject(linkHandle, &linkTarget, NULL);

    deviceName[linkTarget.Length / sizeof(WCHAR)] = L'\0';

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


VOID CacheFiles( CHAR driveLetter )
{
    HANDLE threadHandle;
    FILE_LIST_ENTRY *file = (FILE_LIST_ENTRY*) PushFileList;

    //create copy progress window

    NtCreateThreadEx(
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NtCurrentProcess(),
        &CpwThread,
        NULL,
        NoThreadFlags,
        0, 0, 0,
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

    name = (WCHAR*)Memory_Allocate(
            /*PushHeapHandle,
            0,*/
            (String_GetLength(FileEntry->Name) + 1) * sizeof(WCHAR)
            );

    String_Copy(name, FileEntry->Name);

    if (*FileList == NULL)
    {
        FILE_LIST_ENTRY *fileList;

        *FileList = (FILE_LIST)Memory_Allocate(
            /*PushHeapHandle,
            0,*/
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

    fileListEntry->NextEntry = (FILE_LIST_ENTRY *)Memory_Allocate(
        /*PushHeapHandle,
        0,*/
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
        Log(L"Folder not exist!");
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

    if (Game_IsGame(fileName))
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
        PushSharedMemory->VsyncOverrideMode = game.Settings.VsyncOverrideMode;

        if (game.Settings.FrameLimit > 1)
        {
            PushSharedMemory->FrameLimit = game.Settings.FrameLimit;
        }

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

        //mij
        SetProfile(game.Name);

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


typedef unsigned __int64 DWORD64;
DWORD64 GetRemoteModuleHandle(HANDLE ProcessHandle, WCHAR* ModuleName);
DWORD64 GetRemoteProcAddress(HANDLE ProcessHandle, DWORD64 BaseAddress, const char* name_ord);
DWORD64 NtCreateThreadEx64(HANDLE ProcessHandle, DWORD64 StartRoutine, DWORD RemoteMemory);


VOID Inject( HANDLE ProcessHandle )
{
    VOID *remoteMemory;
    DWORD64 kernel32Base;
    DWORD64 threadHandle;
    WCHAR modulePath[260], *pszLastSlash;
    UINT32 regionSize;
    BOOLEAN x64 = FALSE;

    if (!ProcessHandle) return;

    if (!Process_IsWow64(ProcessHandle))
    {
        x64 = TRUE;
    }

    Resource_Extract(x64 ? L"OVERLAY64" : L"OVERLAY32", x64 ? PUSH_LIB_NAME_64 : PUSH_LIB_NAME_32);
    String_Copy(modulePath, PushFilePath);

    pszLastSlash = String_FindLastChar(modulePath, '\\');
    pszLastSlash[1] = '\0';

    String_Concatenate(modulePath, x64 ? PUSH_LIB_NAME_64 : PUSH_LIB_NAME_32);

    // Allocate memory in the remote process's address space.
    remoteMemory = NULL;
    regionSize = sizeof(modulePath);

    NtAllocateVirtualMemory(ProcessHandle, &remoteMemory, 0, &regionSize, MEM_COMMIT, PAGE_READWRITE);

    // Copy library name
    Process_WriteMemory(ProcessHandle, remoteMemory, modulePath, sizeof(modulePath));

    // Load dll into the remote process
    if (x64)
    {
        DWORD64 _LoadLibraryW;

        kernel32Base = GetRemoteModuleHandle(ProcessHandle, L"kernel32.dll");

        if (!kernel32Base)
        {
            Log(L"Failed to get kernel32.dll base address");
            return;
        }

        _LoadLibraryW = GetRemoteProcAddress(ProcessHandle, kernel32Base, "LoadLibraryW");

        if (!_LoadLibraryW)
        {
            Log(L"Failed to get address of LoadLibraryW");
            return;
        }

        threadHandle = NtCreateThreadEx64(ProcessHandle, _LoadLibraryW, (DWORD)remoteMemory);

        Log(L"threadHandle: 0x%llX", threadHandle);
    }
    else
    {
        HANDLE _threadHandle;
        void* _LoadLibraryW;

        kernel32Base = Module_GetHandle(L"kernel32.dll");
        _LoadLibraryW = Module_GetProcedureAddress((VOID*)kernel32Base, "LoadLibraryW");

        NtCreateThreadEx(
            &_threadHandle,
            THREAD_ALL_ACCESS,
            NULL,
            ProcessHandle,
            _LoadLibraryW,
            remoteMemory,
            NoThreadFlags,
            0, 0, 0,
            NULL
            );

        threadHandle = _threadHandle;
    }

    NtWaitForSingleObject((VOID*)threadHandle, FALSE, NULL);

    // Clean up

    NtClose((HANDLE)threadHandle);

    // Free the memory we allocated inside the remote process
    regionSize = 0;

    NtFreeVirtualMemory(ProcessHandle, &remoteMemory, &regionSize, MEM_RELEASE);
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


HANDLE OpenProcess( DWORD ProcessId )
{
    HANDLE processHandle = 0;

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
        securityDescriptor = (SECURITY_DESCRIPTOR*)Memory_Allocate(bufferSize);

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
            securityDescriptor = (SECURITY_DESCRIPTOR*)Memory_Allocate(bufferSize);

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
            return NULL;
        }

        // Open it with WRITE_DAC access so that we can write to the DACL.
        processHandle = Process_Open(ProcessId, (0x00040000L)); //WRITE_DAC

        if (processHandle == 0)
        {
            Memory_Free(securityDescriptor);
            return NULL;
        }

        status = NtSetSecurityObject(
            processHandle,
            DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
            securityDescriptor
            );

        if (!NT_SUCCESS(status))
        {
            Memory_Free(securityDescriptor);
            return NULL;
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
        return NULL;
    }

    return processHandle;
}


VOID CreateOverlay( DWORD ProcessId )
{
    VOID *processHandle = 0;
    wchar_t filePath[260];
    wchar_t *executableName;
    static int lastProcessId = 0;
    PUSH_GAME *game = NULL;

    if (lastProcessId == ProcessId)
        return;

    lastProcessId = ProcessId;

    processHandle = OpenProcess(ProcessId);

    if (!processHandle)
    {
        Log(L"Failed to get handle for PID %i", ProcessId);

        return;
    }

    Process_GetFileNameByHandle(processHandle, filePath);

    if (Game_IsGame(filePath))
    {
        game = Memory_AllocateEx(sizeof(PUSH_GAME), HEAP_ZERO_MEMORY);

        Game_Initialize(filePath, game);
    }

    if (game && game->Settings.DisableOverlay)
    {
        Log(L"Skipping injection on %s", game->ExecutableName);

        return;
    }

    executableName = String_FindLastChar(filePath, '\\');
    executableName++;

    Log(L"Injecting into %s", executableName);

    GameProcessId = ProcessId;

    if (PushOverlayInterface == OVERLAY_INTERFACE_PURE)
    {
        Inject(processHandle);
    }
    else
    {
        if (!Process_GetId(L"RTSS.exe", 0))
        {
            MessageBoxW(0, L"Rivatuner Statistics Server not running!", 0, 0);
        }
    }

    Process_Close(processHandle);

    //start timer
    SetTimer(PushMainWindow->Handle, 0, 1000, 0);

    // Start disk monitoring;
    if (!DiskMonitorInitialized)
        DiskStartMonitoring();
}


typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

NTSTATUS __stdcall NtCreateEvent(
    HANDLE* EventHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes,
    EVENT_TYPE EventType,
    BOOLEAN InitialState
    );
#define EVENT_ALL_ACCESS    0x1f0003


DWORD __stdcall RetrieveProcessEvent( VOID* Parameter )
{
    OVERLAPPED              ov          = { 0 };
    //BOOLEAN                    bReturnCode = FALSE;
    UINT32                  iBytesReturned;
    PROCESS_CALLBACK_INFO   processInfo;


    // Create an event handle for async notification from the driver
    NtCreateEvent(&ov.hEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

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
    NtCreateEvent(&ov.hEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

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

    CreateOverlay(imageInfo.processID);
    NtClose(ov.hEvent);

    return 0;
}


NTSTATUS __stdcall NtOpenEvent(
    HANDLE* EventHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes
    );


HANDLE OpenEvent( WCHAR* EventName )
{
    UNICODE_STRING eventName;
    OBJECT_ATTRIBUTES objAttrib;
    HANDLE eventHandle;

    UnicodeString_Init(&eventName, EventName);

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = BaseGetNamedObjectDirectory();
    objAttrib.ObjectName = &eventName;
    objAttrib.Attributes = 0;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    NtOpenEvent(&eventHandle, SYNCHRONIZE, &objAttrib);

    return eventHandle;
}


typedef enum _WAIT_TYPE
{
    WaitAll,
    WaitAny,
    WaitNotification
} WAIT_TYPE;

NTSTATUS __stdcall NtWaitForMultipleObjects(
    ULONG Count,
    HANDLE Handles[],
    WAIT_TYPE WaitType,
    BOOLEAN Alertable,
    LARGE_INTEGER* Timeout
    );


DWORD __stdcall MonitorThread(VOID* Parameter)
{
    HANDLE processEvent;
    HANDLE d3dImageEvent;
    VOID *handles[2];

    processEvent = OpenEvent(L"Global\\" PUSH_PROCESS_EVENT_NAME);
    d3dImageEvent = OpenEvent(L"Global\\" PUSH_IMAGE_EVENT_NAME);

    handles[0] = processEvent;
    handles[1] = d3dImageEvent;

    while (processEvent)
    {
        NTSTATUS result;
        HANDLE threadHandle;

        result = NtWaitForMultipleObjects(2, &handles[0], WaitAny, FALSE, NULL);

        if (processEvent && handles[result - STATUS_WAIT_0] == processEvent)
        {
            NtCreateThreadEx(
                &threadHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                &RetrieveProcessEvent,
                NULL,
                NoThreadFlags,
                0, 0, 0,
                NULL
                );
        }
        else if (handles[result - STATUS_WAIT_0] == d3dImageEvent)
        {
            NtCreateThreadEx(
                &threadHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                &RetrieveImageEvent,
                NULL,
                NoThreadFlags,
                0, 0, 0,
                NULL
                );
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
        HANDLE* FileHandle,
        ULONG DesiredAccess,
        OBJECT_ATTRIBUTES* ObjectAttributes,
        PIO_STATUS_BLOCK IoStatusBlock,
        ULONG ShareAccess,
        ULONG CreateDisposition,
        ULONG CreateOptions,
        ULONG NamedPipeType,
        ULONG ReadMode,
        ULONG CompletionMode,
        ULONG MaximumInstances,
        ULONG InboundQuota,
        ULONG OutboundQuota,
        LARGE_INTEGER* DefaultTimeout
        );


#include "Hardware\GPU\AMD\adl.h"
#define PIPE_ACCEPT_REMOTE_CLIENTS 0x00000000
#define _ADDRESSOF(v)   ( &(v) )
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define va_end(ap)      ( ap = (va_list)0 )


VOID Log(const wchar_t* Format, ...)
{
    wchar_t buffer[260];
    wchar_t output[260];
    va_list _Arglist;

    String_Copy(output, L"[PUSH] ");
    va_start(_Arglist, Format);
    vswprintf_s(buffer, 260, Format, _Arglist);
    va_end(_Arglist);

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
                    GPU_SetEngineClock(PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax);
                    GPU_SetMemoryClock(PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax);
                    GPU_SetVoltage(PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax);
                }
                else if (String_Compare(buffer, L"UpdateFanSpeed") == 0)
                {
                    Adl_SetFanDutyCycle(PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle);
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


typedef enum _SECTION_INFORMATION_CLASS
{
    SectionBasicInformation,
    SectionImageInformation,
    SectionRelocationInformation, // name:wow64:whNtQuerySection_SectionRelocationInformation
    SectionOriginalBaseInformation, // PVOID BaseAddress
    MaxSectionInfoClass
} SECTION_INFORMATION_CLASS;
NTSTATUS __stdcall NtQuerySection(
    HANDLE SectionHandle,
    SECTION_INFORMATION_CLASS SectionInformationClass,
    VOID* SectionInformation,
    SIZE_T SectionInformationLength,
    SIZE_T* ReturnLength
    );

typedef struct _SECTION_BASIC_INFORMATION
{
    VOID* BaseAddress;
    ULONG AllocationAttributes;
    LARGE_INTEGER MaximumSize;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;
double PCFreq = 0.0;


VOID StartCounter()
{
    LARGE_INTEGER perfCount;
    LARGE_INTEGER frequency;

    NtQueryPerformanceCounter(&perfCount, &frequency);

    PCFreq = (double)frequency.QuadPart / 1000.0;;
}


double GetPerformanceCounter()
{
    LARGE_INTEGER li;

    NtQueryPerformanceCounter(&li, NULL);

    return (double)li.QuadPart / PCFreq;
}


typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    union
    {
        LIST_ENTRY InInitializationOrderLinks;
        LIST_ENTRY InProgressLinks;
    };
    VOID* DllBase;
    VOID* EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;


VOID ProcessEnum( SYSTEM_PROCESS_INFORMATION* ProcessInformation )
{
    GAME_LIST gameList;
    PUSH_GAME* game;
    UINT8 i;

    i = 0;

    gameList = Game_GetGames();

    while (gameList != NULL)
    {
        game = gameList->Game;

        if (ProcessInformation->ImageName.Length 
            && String_CompareN(
                game->ExecutableName, 
                ProcessInformation->ImageName.Buffer, 
                ProcessInformation->ImageName.Length) == 0)
        {
            CreateOverlay((DWORD)ProcessInformation->UniqueProcessId);
        }

        gameList = gameList->NextEntry;

        i++;
    }
}


INT32 __stdcall start( )
{
    HANDLE sectionHandle, *hMutex;
    HANDLE eventHandle;
    HANDLE threadHandle;
    DWORD sectionSize;
    MSG messages;
    OBJECT_ATTRIBUTES objAttrib = {0};
    PTEB threadEnvironmentBlock;
    UNICODE_STRING eventSource;
    LDR_DATA_TABLE_ENTRY *module;
    SECTION_BASIC_INFORMATION sectionInfo;
    LARGE_INTEGER newSectionSize;

    InitializeCRT();

    threadEnvironmentBlock = NtCurrentTeb();

    PushProcessId = threadEnvironmentBlock->ClientId.UniqueProcess;
    PushHeapHandle = threadEnvironmentBlock->ProcessEnvironmentBlock->ProcessHeap;
    PushSessionId = threadEnvironmentBlock->ProcessEnvironmentBlock->SessionId;

    // Check if already running
    hMutex = CreateMutexW(0, FALSE, L"PushOneInstance");

    if (threadEnvironmentBlock->LastErrorValue == ERROR_ALREADY_EXISTS
        || threadEnvironmentBlock->LastErrorValue == ERROR_ACCESS_DENIED)
    {
        MessageBoxW(0, L"Only one instance!", 0,0);
        ExitProcess(0);
    }


    //create image event
    eventHandle = NULL;

    UnicodeString_Init(&eventSource, L"Global\\" PUSH_IMAGE_EVENT_NAME);

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = BaseGetNamedObjectDirectory();
    objAttrib.ObjectName = &eventSource;
    objAttrib.Attributes = OBJ_OPENIF;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    NtCreateEvent(&eventHandle, EVENT_ALL_ACCESS, &objAttrib, NotificationEvent, FALSE);

    // populate file name and path
    module = (LDR_DATA_TABLE_ENTRY*)threadEnvironmentBlock->ProcessEnvironmentBlock->Ldr->InLoadOrderModuleList.Flink;

    Memory_Copy(PushFilePath, module->FullDllName.Buffer, module->FullDllName.Length);

    PushFilePath[module->FullDllName.Length] = L'\0';

    // Start Driver.
    Driver_Extract();
    Driver_Load();

    //initialize instance
    PushInstance = Module_GetHandle(L"Push.exe");

    // Create interface
    MwCreateMainWindow();

    // Create section.
    sectionSize = sizeof(PUSH_SHARED_MEMORY) + OSD_GetSize();

    PushSharedMemory = (PUSH_SHARED_MEMORY*)Memory_MapViewOfSection(PUSH_SECTION_NAME, sectionSize, &sectionHandle);

    if (!PushSharedMemory)
    {
        Log(L"Could not create shared memory");
        return 0;
    }

    Log(L"Created section of size %i bytes", sectionSize);

    //zero struct
    Memory_Clear(PushSharedMemory, sizeof(PUSH_SHARED_MEMORY));

    //initialize window handle used by overlay
    //PushSharedMemory->WindowHandle = PushMainWindow->Handle;

    if (File_Exists(PUSH_SETTINGS_FILE))
    {
        wchar_t *buffer;
        wchar_t marker;

        // Check if file is UTF-16LE.
        buffer = (WCHAR*) File_Load(PUSH_SETTINGS_FILE, NULL);
        marker = buffer[0];

        Memory_Free(buffer);

        if (marker == 0xFEFF)
            //is UTF-LE.
        {
            // Init settings from ini file.

            buffer = Memory_Allocate(100 * sizeof(WCHAR));

            if (Ini_ReadBoolean(L"Settings", L"FrameLimit", FALSE, L".\\" PUSH_SETTINGS_FILE))
                PushSharedMemory->FrameLimit = TRUE;

            if (Ini_ReadBoolean(L"Settings", L"ThreadOptimization", FALSE, L".\\" PUSH_SETTINGS_FILE))
                PushSharedMemory->ThreadOptimization = TRUE;

            if (Ini_ReadBoolean(L"Settings", L"KeepFps", FALSE, L".\\" PUSH_SETTINGS_FILE))
                PushSharedMemory->KeepFps = TRUE;

            Ini_GetString(L"Settings", L"OverlayInterface", NULL, buffer, 5, L".\\" PUSH_SETTINGS_FILE);

            if (String_Compare(buffer, L"PURE") == 0)
                PushOverlayInterface = OVERLAY_INTERFACE_PURE;
            else if (String_Compare(buffer, L"RTSS") == 0)
                PushOverlayInterface = OVERLAY_INTERFACE_RTSS;

            Ini_GetString(L"Settings", L"KeyboardHookType", L"AUTO", buffer, 10, L".\\" PUSH_SETTINGS_FILE);

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

            Ini_GetString(L"Settings", L"EngineClockMax", NULL, buffer, 5, L".\\" PUSH_SETTINGS_FILE);
            PushSharedMemory->HarwareInformation.DisplayDevice.EngineOverclock = _wtoi(buffer);

            Ini_GetString(L"Settings", L"MemoryClockMax", NULL, buffer, 5, L".\\" PUSH_SETTINGS_FILE);
            PushSharedMemory->HarwareInformation.DisplayDevice.MemoryOverclock = _wtoi(buffer);

            Ini_GetString(L"Settings", L"ControllerTimeout", NULL, buffer, 5, L".\\" PUSH_SETTINGS_FILE);
            PushSharedMemory->ControllerTimeout = _wtoi(buffer);

            Ini_GetString(L"Settings", L"FontName", NULL, buffer, 100, L".\\" PUSH_SETTINGS_FILE);
            String_Copy(PushSharedMemory->FontName, buffer);

            Memory_Free(buffer);

            if (Ini_ReadBoolean(L"Settings", L"FontBold", FALSE, L".\\" PUSH_SETTINGS_FILE))
                PushSharedMemory->FontBold = TRUE;
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

    NtQuerySection(
        sectionHandle,
        SectionBasicInformation,
        &sectionInfo,
        sizeof(SECTION_BASIC_INFORMATION),
        NULL
        );

    newSectionSize.QuadPart = OSD_Initialize() + sizeof(PUSH_SHARED_MEMORY);

    if (newSectionSize.QuadPart > sectionInfo.MaximumSize.QuadPart)
    {
        Log(L"Shared memory too small!");
    }

    // Check for running games
    Process_EnumProcesses(ProcessEnum);

    // Activate process monitoring

    PushToggleProcessMonitoring(TRUE);

    g_szPrevGame[5] = '\0';

    NtCreateThreadEx(
        &PushMonitorThreadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NtCurrentProcess(),
        &MonitorThread,
        NULL,
        NoThreadFlags,
        0, 0, 0,
        NULL
        );

    NtCreateThreadEx(
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NtCurrentProcess(),
        &PipeThread,
        NULL,
        NoThreadFlags,
        0, 0, 0,
        NULL
        );

    // Handle messages

    while(GetMessageW(&messages, 0,0,0))
    {
        TranslateMessage(&messages);

        DispatchMessageW(&messages);
    }

    ExitProcess(0);

    return 0;
}


VOID PushOnTimer()
{
    RefreshHardwareInfo();
    //UpdateSharedMemory();
    OSD_Refresh();
}


typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef struct _KUSER_SHARED_DATA
{
    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;

    volatile KSYSTEM_TIME InterruptTime;
    volatile KSYSTEM_TIME SystemTime;
} KUSER_SHARED_DATA;

#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)0x7ffe0000)


VOID NtGetSystemTimeAsFileTime(FILETIME* lpFileTime)
{
    LARGE_INTEGER SystemTime;

    do
    {
        SystemTime.u.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.u.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.u.HighPart != USER_SHARED_DATA->SystemTime.High2Time);

    lpFileTime->dwLowDateTime = SystemTime.u.LowPart;
    lpFileTime->dwHighDateTime = SystemTime.u.HighPart;
}


#define DAYSPERYEAR 365
#define DAYSPER4YEARS (4*DAYSPERYEAR+1)
#define DAYSPER100YEARS (25*DAYSPER4YEARS-1)
#define DAYSPER400YEARS (4*DAYSPER100YEARS+1)
#define SECONDSPERDAY (24*60*60)
#define SECONDSPERHOUR (60*60)
#define LEAPDAY 59

#define DIFFTIME 0x19db1ded53e8000ULL
#define DIFFDAYS (3 * DAYSPER100YEARS + 17 * DAYSPER4YEARS + 1 * DAYSPERYEAR)


__int64 FileTimeToUnixTime( FILETIME *FileTime )
{
    ULARGE_INTEGER ULargeInt;
    __int64 time;

    ULargeInt.u.LowPart = FileTime->dwLowDateTime;
    ULargeInt.u.HighPart = FileTime->dwHighDateTime;
    ULargeInt.QuadPart -= DIFFTIME;

    time = ULargeInt.QuadPart / 10000000;

    return time;
}
//#include <time.h>
long leapyears_passed(long days)
{
    long quadcenturies, centuries, quadyears;
    quadcenturies = days / DAYSPER400YEARS;
    days -= quadcenturies;
    centuries = days / DAYSPER100YEARS;
    days += centuries;
    quadyears = days / DAYSPER4YEARS;
    return quadyears - centuries + quadcenturies;
}
long leapdays_passed(long days)
{
    return leapyears_passed(days + DAYSPERYEAR - LEAPDAY + 1);
}


unsigned int g_monthdays[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
unsigned int g_lpmonthdays[13] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

typedef struct _TIME_INFORMATION {
    int tm_sec;     /* seconds after the minute - [0,59] */
    int tm_min;     /* minutes after the hour - [0,59] */
    int tm_hour;    /* hours since midnight - [0,23] */
    int tm_mday;    /* day of the month - [1,31] */
    int tm_mon;     /* months since January - [0,11] */
    int tm_year;    /* years since 1900 */
    int tm_wday;    /* days since Sunday - [0,6] */
    int tm_yday;    /* days since January 1 - [0,365] */
    int tm_isdst;   /* daylight savings time flag */
}TIME_INFORMATION;


void GetTimeUnits( TIME_INFORMATION *ptm, __int64 time )
{
    unsigned int days, daystoyear, dayinyear, leapdays, leapyears, years, month;
    unsigned int secondinday, secondinhour;
    unsigned int *padays;

    /* Divide into date and time */
    days = (unsigned int)(time / SECONDSPERDAY);
    secondinday = time % SECONDSPERDAY;

    /* Shift to days from 1.1.1601 */
    days += DIFFDAYS;

    /* Calculate leap days passed till today */
    leapdays = leapdays_passed(days);

    /* Calculate number of full leap years passed */
    leapyears = leapyears_passed(days);

    /* Are more leap days passed than leap years? */
    if (leapdays > leapyears)
    {
        /* Yes, we're in a leap year */
        padays = g_lpmonthdays;
    }
    else
    {
        /* No, normal year */
        padays = g_monthdays;
    }

    /* Calculate year */
    years = (days - leapdays) / 365;
    ptm->tm_year = years - 299;

    /* Calculate number of days till 1.1. of this year */
    daystoyear = years * 365 + leapyears;

    /* Calculate the day in this year */
    dayinyear = days - daystoyear;

    /* Shall we do DST corrections? */
    ptm->tm_isdst = 0;

    ptm->tm_yday = dayinyear;

    /* dayinyear < 366 => terminates with i <= 11 */
    for (month = 0; dayinyear >= padays[month + 1]; month++)
        ;

    /* Set month and day in month */
    ptm->tm_mon = month;
    ptm->tm_mday = 1 + dayinyear - padays[month];

    /* Get weekday */
    ptm->tm_wday = (days + 1) % 7;

    /* Calculate hour and second in hour */
    ptm->tm_hour = secondinday / SECONDSPERHOUR;
    secondinhour = secondinday % SECONDSPERHOUR;

    /* Calculate minute and second */
    ptm->tm_min = secondinhour / 60;
    ptm->tm_sec = secondinhour % 60;
}
int wcsftime(wchar_t *str, int max, const wchar_t *format,
    TIME_INFORMATION *mstm);


VOID Push_FormatTime( WCHAR* Buffer )
{
    __int64 rawtime;
    FILETIME Now;
    TIME_INFORMATION timeInfo;


    NtGetSystemTimeAsFileTime(&Now);
    rawtime = FileTimeToUnixTime(&Now);

    GetTimeUnits(&timeInfo, rawtime);
    String_Format(Buffer, 20, L"%i:%i:%i", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
}


TYPE_iswspace       iswspace;
TYPE_memcmp         memcmp;
TYPE_memcpy         memcpy;
TYPE_memset         memset;
TYPE_strcmp         strcmp;
TYPE_strcpy         strcpy;
TYPE_strlen         strlen;
TYPE_strncmp        strncmp;
TYPE_strncpy        strncpy;
TYPE_swscanf_s      swscanf_s;
TYPE_vswprintf_s    vswprintf_s;
TYPE_wcsncat        wcsncat;
TYPE_wcsnlen        wcsnlen;
TYPE_wcstol         wcstol;
TYPE__wtoi          _wtoi;

TYPE_strcmp         ntdll_strcmp;
TYPE_memcmp         ntdll_memcmp;
TYPE_strncmp        ntdll_strncmp;
TYPE_strlen         ntdll_strlen;


FARPROC __stdcall GetProcAddress(
    HANDLE hModule,
    CHAR*  lpProcName
    );

int _fltused;


VOID InitializeCRT()
{
    void* ntdll;

    ntdll = Module_GetHandle(L"ntdll.dll");

    iswspace = (TYPE_iswspace)GetProcAddress(ntdll, "iswspace");
    memcmp = ntdll_memcmp= (TYPE_memcmp)GetProcAddress(ntdll, "memcmp");
    memcpy = (TYPE_memcpy)GetProcAddress(ntdll, "memcpy");
    memset = (TYPE_memset)GetProcAddress(ntdll, "memset");
    strcmp = ntdll_strcmp= (TYPE_strcmp)GetProcAddress(ntdll, "strcmp");
    strcpy = (TYPE_strcpy)GetProcAddress(ntdll, "strcpy");
    strlen = ntdll_strlen= (TYPE_strlen)GetProcAddress(ntdll, "strlen");
    strncmp = ntdll_strncmp= (TYPE_strncmp)GetProcAddress(ntdll, "strncmp");
    strncpy = (TYPE_strncpy)GetProcAddress(ntdll, "strncpy");
    swscanf_s = (TYPE_swscanf_s)GetProcAddress(ntdll, "swscanf_s");
    vswprintf_s = (TYPE_vswprintf_s)GetProcAddress(ntdll, "vswprintf_s");
    wcsncat = (TYPE_wcsncat)GetProcAddress(ntdll, "wcsncat");
    wcsnlen = (TYPE_wcsnlen)GetProcAddress(ntdll, "wcsnlen");
    wcstol = (TYPE_wcstol)GetProcAddress(ntdll, "wcstol");
    _wtoi = (TYPE__wtoi)GetProcAddress(ntdll, "_wtoi");
}


