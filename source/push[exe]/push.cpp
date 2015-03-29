#include <sl.h>
#include <slgui.h>
#include <pushbase.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <slc.h>
#include <slfile.h>
#include <sldriver.h>
#include <gui.h>
#include <main.h>
#include <copy.h>
#include "push.h"
#include <slini.h>
#include "file.h"
#include "ring0.h"


#include "RAMdisk\ramdisk.h"
#include "Hardware\hwinfo.h"
#include "batch.h"
#include "driver.h"


CHAR g_szDllDir[260];
WCHAR g_szPrevGame[260];
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
UINT32  PushPageSize;
WCHAR g_szLastDir[260];

//BfBatchFile* PushCacheBatchFile;


typedef long (__stdcall *TYPE_NtSuspendProcess)( VOID* hProcessHandle );
typedef long (__stdcall *TYPE_NtResumeProcess)( VOID* hProcessHandle );
typedef int (__stdcall* TYPE_MuteJack)(CHAR *pin);


TYPE_NtSuspendProcess NtSuspendProcess;
TYPE_NtResumeProcess NtResumeProcess;
TYPE_MuteJack MuteJack;


extern "C" VOID* SlOpenProcess(
    UINT16 processID,
    DWORD rights);

extern "C" DWORD __stdcall MapFileAndCheckSumW(
    _In_   WCHAR* Filename,
    _Out_  DWORD* HeaderSum,
    _Out_  DWORD* CheckSum
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
        wchar_t *executable = SlStringFindLastChar(ExecutablePath, '\\');

        executable++;

        MapFileAndCheckSumW(ExecutablePath, &headerSum, &checkSum);

        while (gameList != NULL)
        {
            if (SlStringCompare(gameList->Game->ExecutableName, executable) == 0)
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


extern "C" INTBOOL __stdcall SetWindowTextW(
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

    pszFileName = SlStringFindLastChar(FileName, '\\') + 1;

    if (!bMarkedForCaching)
        // file was a member of a folder marked for caching
    {
        SlStringConcatenate(destination, g_szLastDir);
        SlStringConcatenate(destination, L"\\");
    }

    SlStringConcatenate(destination, pszFileName);

    SlFileCopy(FileName, destination, CopyProgress);

    SlStringCopy(dosName, FileName);

    slash = SlStringFindChar(dosName, '\\');
    *slash = L'\0';

    QueryDosDeviceW(dosName, deviceName, 260);

    SlStringConcatenate(deviceName, L"\\");
    SlStringConcatenate(deviceName, slash + 1);

    R0QueueFile(
        deviceName,
        SlStringGetLength(deviceName) + 1
             );
}


extern "C" NTSTATUS __stdcall NtTerminateThread(
    VOID* ThreadHandle,
    NTSTATUS ExitStatus
    );


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


LONG
NormalizeNTPath( WCHAR* pszPath, size_t nMax )
// Normalizes the path returned by GetProcessImageFileName
{
    CHAR cSave, cDrive;
    WCHAR szNTPath[_MAX_PATH], szDrive[_MAX_PATH] = L"A:";
    //WCHAR *pszSlash = wcschr(&pszPath[1], '\\');
    WCHAR *pszSlash = SlStringFindChar(&pszPath[1], '\\');

    if (pszSlash) pszSlash = SlStringFindChar(pszSlash+1, '\\');
    if (!pszSlash)
        return E_FAIL;
    cSave = *pszSlash;
    *pszSlash = 0;

    // We'll need to query the NT device names for the drives to find a match with pszPath

    for (cDrive = 'A'; cDrive < 'Z'; ++cDrive)
    {
        szDrive[0] = cDrive;
        szNTPath[0] = 0;
        if (0 != QueryDosDeviceW(szDrive, szNTPath, 260) &&
            0 == SlStringCompareN(szNTPath, pszPath, 260))
        {
            // Match
            SlStringConcatenate(szDrive, L"\\");
            SlStringConcatenate(szDrive, pszSlash+1);

            SlStringCopy(pszPath, szDrive);

            return S_OK;
        }
    }


    *pszSlash = cSave;
    return E_FAIL;
}


VOID
PushAddToFileList( FILE_LIST* FileList, FILE_LIST_ENTRY *FileEntry )
{
    FILE_LIST_ENTRY *fileListEntry;
    WCHAR *name;

    name = (WCHAR*) RtlAllocateHeap(
            PushHeapHandle,
            0,
            (SlStringGetLength(FileEntry->Name) + 1) * sizeof(WCHAR)
            );

    SlStringCopy(name, FileEntry->Name);

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


VOID
Cache( PUSH_GAME* Game )
{
    CHAR mountPoint = 0;
    UINT64 bytes = 0, availableMemory = 0; // in bytes
    SYSTEM_BASIC_PERFORMANCE_INFORMATION performanceInfo;
    BfBatchFile batchFile(Game);

    // Check if game is already cached so we donot have wait through another
    if (SlStringCompare(g_szPrevGame, Game->InstallPath) == 0 && !g_bRecache)
        return;

    g_bRecache = FALSE;

    if (!FolderExists(Game->InstallPath))
    {
        MessageBoxW(0, L"Folder not exist!", 0,0);

        return;
    }

    // Check available memory
    NtQuerySystemInformation(
        SystemBasicPerformanceInformation,
        &performanceInfo,
        sizeof(SYSTEM_BASIC_PERFORMANCE_INFORMATION),
        NULL
        );

    availableMemory = performanceInfo.CommitLimit - performanceInfo.CommittedPages;
    availableMemory *= PushPageSize;

    // Read batch file
    PushFileList = 0;

    bytes = batchFile.GetBatchSize();
    PushFileList = batchFile.GetBatchList();

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
            NULL
            );
    }

    RemoveRamDisk();

    mountPoint = FindFreeDriveLetter();

    CreateRamDisk(bytes, mountPoint);
    FormatRamDisk();
    CacheFiles(mountPoint);

    // Release batchfile list
    PushFileList = NULL;

    SlStringCopy(g_szPrevGame, Game->InstallPath);
}


VOID
OnProcessEvent( UINT16 processID )
{
    WCHAR fileName[260];
    VOID *processHandle = NULL;
    UINT32 iBytesRead;
    WCHAR *result = 0;
    CHAR szCommand[] = "MUTEPIN 1 a";
    
    processHandle = SlOpenProcess(processID, PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME);

    if (!processHandle)
        return;

    GetProcessImageFileNameW(processHandle, fileName, 260);
    NormalizeNTPath(fileName, 260);

    if (IsGame(fileName))
    {
        PUSH_GAME game = { 0 };

        Game_Initialize(fileName, &game);

        if (game.GameSettings.UseRamDisk)
        {
            PushSharedMemory->GameUsesRamDisk = TRUE;

            //suspend process to allow us time to cache files
            NtSuspendProcess(processHandle);
            Cache(&game);
        }

        PushSharedMemory->DisableRepeatKeys = game.GameSettings.DisableRepeatKeys;
        PushSharedMemory->SwapWASD = game.GameSettings.SwapWASD;
        PushSharedMemory->VsyncOverrideMode = game.GameSettings.VsyncOverrideMode;

        // Check if user wants maximum gpu engine and memory clocks
        if (SlIniReadBoolean(L"Settings", L"ForceMaxClocks", FALSE, L".\\" PUSH_SETTINGS_FILE))
            HwForceMaxClocks();

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
            NtResumeProcess(processHandle);
    }
    else
    {
        PushSharedMemory->GameUsesRamDisk = FALSE;
    }

    NtClose(processHandle);
}


VOID
Inject32( VOID *hProcess )
{
    VOID *threadHandle, *pLibRemote, *hKernel32;
    WCHAR szModulePath[260], *pszLastSlash;

    hKernel32 = GetModuleHandleW(L"Kernel32");

    GetModuleFileNameW(0, szModulePath, 260);

    pszLastSlash = SlStringFindLastChar(szModulePath, '\\');

    pszLastSlash[1] = '\0';

    SlStringConcatenate(szModulePath, PUSH_LIB_NAME_32);

    // Allocate remote memory
    pLibRemote = VirtualAllocEx(hProcess, 0, sizeof(szModulePath), MEM_COMMIT, PAGE_READWRITE);

    // Copy library name
    WriteProcessMemory(hProcess, pLibRemote, szModulePath, sizeof(szModulePath), 0);

    // Load dll into the remote process
    threadHandle = CreateRemoteThread(hProcess,
                                 0,0,
                                 (PTHREAD_START_ROUTINE) GetProcAddress(hKernel32, "LoadLibraryW"),
                                 pLibRemote,
                                 0,0);

    WaitForSingleObject(threadHandle, INFINITE );

    // Clean up
    //CloseHandle(hThread);
    NtClose(threadHandle);

    VirtualFreeEx(hProcess, pLibRemote, sizeof(szModulePath), MEM_RELEASE);
}

#include <stdio.h>
#include <stdarg.h>
extern "C" void __stdcall OutputDebugStringA(
  _In_opt_  CHAR* lpOutputString
);
void __cdecl SlDebugPrint(CHAR* szFormat, ...)
{
    char strA[4096];
    char strB[4096];

    va_list ap;
    va_start(ap, szFormat);
    vsprintf_s(strA, sizeof(strA), szFormat, ap);
    strA[4095] = '\0';
    va_end(ap);

    sprintf_s(strB, sizeof(strB), "[PUSH] %s\r\n", strA);

    strB[4095] = '\0';

    OutputDebugStringA(strB);
}

    extern "C" INTBOOL __stdcall IsWow64Process(
  _In_   HANDLE hProcess,
  _Out_  INTBOOL* Wow64Process
);
    VOID Inject64( UINT16 ProcessId, WCHAR* Path );
VOID
OnImageEvent( UINT16 ProcessId )
{
    VOID *processHandle = 0;
    INTBOOL isWow64;

    processHandle = SlOpenProcess(
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
        ACL *dacl;
        VOID *secdesc;
        // Get the DACL of this process since we know we have
          // all rights in it. This really can't fail.
          if(GetSecurityInfo(NtCurrentProcess(),
                             SE_KERNEL_OBJECT,
                             (0x00000004L), //DACL_SECURITY_INFORMATION,
                             0,
                             0,
                             &dacl,
                             0,
                             &secdesc) != ERROR_SUCCESS)
          {
             return;
          }

          // Open it with WRITE_DAC access so that we can write to the DACL.
          processHandle = SlOpenProcess(ProcessId, (0x00040000L)); //WRITE_DAC

          if(processHandle == 0)
          {
             LocalFree(secdesc);
             return;
          }

          if(SetSecurityInfo(processHandle,
                             SE_KERNEL_OBJECT,
                             (0x00000004L) |    // DACL_SECURITY_INFORMATION
                             (0x20000000L),     //UNPROTECTED_DACL_SECURITY_INFORMATION,
                             0,
                             0,
                             dacl,
                             0) != ERROR_SUCCESS)
          {
             LocalFree(secdesc);
             return;
          }

          // The DACL is overwritten with our own DACL. We
          // should be able to open it with the requested
          // privileges now.

          //CloseHandle(processHandle);
          NtClose(processHandle);

          processHandle = 0;
          LocalFree(secdesc);

          processHandle = SlOpenProcess(
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
        //OutputDebugStringW(L"Could not get process handle");
        SlDebugPrint("Could not get process handle");
            return;
    }
    SlDebugPrint("ProcessHandle: 0x%x\n", processHandle);
    IsWow64Process(processHandle, &isWow64);
    
    if (isWow64)
    {
        Inject32(processHandle);
    }
    else
    {
        WCHAR szModulePath[260], *pszLastSlash;

        GetModuleFileNameW(0, szModulePath, 260);

        pszLastSlash = SlStringFindLastChar(szModulePath, '\\');
        pszLastSlash[1] = '\0';

        SlStringConcatenate(szModulePath, PUSH_LIB_NAME_64);
        Inject64(ProcessId, szModulePath);
    }

    NtClose(processHandle);
}


VOID
RetrieveProcessEvent()
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

    //CloseHandle(ov.hEvent);
    NtClose(ov.hEvent);
}


VOID
RetrieveImageEvent()
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

    //CloseHandle(ov.hEvent);
    NtClose(ov.hEvent);
}


DWORD __stdcall MonitorThread( VOID* Parameter )
{
    VOID *processEvent, *threadEvent, *d3dImageEvent/*, *ramDiskEvent*/;
    VOID *handles[3];
    processEvent    = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\" PUSH_PROCESS_EVENT_NAME);
    threadEvent     = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\" PUSH_THREAD_EVENT_NAME);
    d3dImageEvent   = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\" PUSH_IMAGE_EVENT_NAME);

    //ramDiskEvent = CreateEventW(0, TRUE, FALSE, L"Local\\" PUSH_RAMDISK_REMOVE_EVENT_NAME);

    handles[0] = processEvent;
    handles[1] = threadEvent;
    handles[2] = d3dImageEvent;
    //handles[3] = ramDiskEvent;

    while (TRUE)
    {
        DWORD result = 0;

        result = WaitForMultipleObjectsEx(
            sizeof(handles)/sizeof(handles[0]),
            &handles[0],
            FALSE,
            INFINITE,
            FALSE
            );

        if (handles[result - WAIT_OBJECT_0] == processEvent)
        {
            RetrieveProcessEvent();
        }
        else if (handles[result - WAIT_OBJECT_0] == d3dImageEvent)
        {
            RetrieveImageEvent();
        }
        /*else if (handles[result - WAIT_OBJECT_0] == threadEvent)
        {
            RetrieveThreadEvent();
        }*/
    }

    return 0;
}


#define DIRECTORY_ALL_ACCESS   (STANDARD_RIGHTS_REQUIRED | 0xF)
#define WRITE_DAC   0x00040000L

#define WRITE_OWNER   0x00080000L


extern "C" 
{

HANDLE __stdcall FindResourceW(
  HANDLE hModule,
  WCHAR* lpName,
  WCHAR* lpType
);


HANDLE __stdcall LoadResource(
  HANDLE hModule,
  HANDLE hResInfo
);


VOID* __stdcall LockResource(
  HANDLE hResData
);


DWORD __stdcall SizeofResource(
  HANDLE hModule,
  HANDLE hResInfo
);

}


extern "C"
BOOLEAN ExtractResource( WCHAR* ResourceName, WCHAR* OutputPath )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;

    HANDLE hResInfo = reinterpret_cast<HANDLE>(
        ::FindResourceW(
            NULL,
            ResourceName,
            ResourceName
        )
    );

    if(!hResInfo) return false;

    HANDLE hResData = ::LoadResource(NULL, hResInfo);

    if(!hResData) return false;

    VOID* pDeskbandBinData = ::LockResource(hResData);

    if(!pDeskbandBinData) return false;

    const DWORD dwDeskbandBinSize = ::SizeofResource(NULL, hResInfo);

    if(!dwDeskbandBinSize) return false;

    status = SlFileCreate(
        &fileHandle,
        OutputPath, 
        FILE_READ_ATTRIBUTES | GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        /*FILE_OVERWRITE_IF*/ FILE_CREATE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status)) return false;

    status = NtWriteFile(
        fileHandle, 
        NULL, 
        NULL, 
        NULL, 
        &isb, 
        pDeskbandBinData, 
        dwDeskbandBinSize, 
        NULL, 
        NULL
        );

    if (!NT_SUCCESS(status)) return false;

    NtClose(fileHandle);

    return true;
}


INT32 __stdcall WinMain( VOID* Instance, VOID *hPrevInstance, CHAR *pszCmdLine, INT32 iCmdShow )
{
    VOID *sectionHandle = INVALID_HANDLE_VALUE, *hMutex;
    MSG messages;
    BOOLEAN bAlreadyRunning;
    OBJECT_ATTRIBUTES objAttrib = {0};
    
    // Check if already running
    hMutex = CreateMutexW(0, FALSE, L"PushOneInstance");

    bAlreadyRunning = (GetLastError() == ERROR_ALREADY_EXISTS ||
                            GetLastError() == ERROR_ACCESS_DENIED);

    if (bAlreadyRunning)
    {
        MessageBoxW(0, L"Only one instance!", 0,0);

        ExitProcess(0);
    }

    thisPID = (UINT32) NtCurrentTeb()->ClientId.UniqueProcess;
    PushHeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    // Extract necessary files.
    
    ExtractResource(L"OVERLAY32", PUSH_LIB_NAME_32);
    ExtractResource(L"OVERLAY64", PUSH_LIB_NAME_64);
    
    Driver_Extract();
    
    // Start Driver.
    Driver_Load();
    
    //initialize instance
    PushInstance = Instance;

    // Create interface
    MwCreateMainWindow();

    // Create file mapping.

    PushSharedMemory = NULL;

    PushSharedMemory = (PUSH_SHARED_MEMORY*) SlCreateFileMapping(
        PUSH_SECTION_NAME, 
        sizeof(PUSH_SHARED_MEMORY)
        );

    if (!PushSharedMemory)
    {
        OutputDebugStringW(L"Could not create shared memory");
        return 0;
    }

    //zero struct
    memset(PushSharedMemory, 0, sizeof(PUSH_SHARED_MEMORY));

    //initialize window handle used by overlay
    PushSharedMemory->WindowHandle = PushMainWindow->Handle;

    if (SlFileExists(PUSH_SETTINGS_FILE))
    {
        WCHAR *buffer;
        
        // Check if file is UTF-16LE.
        buffer = (WCHAR*) SlFileLoad(PUSH_SETTINGS_FILE, NULL);

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

    //start timer
    SetTimer(PushMainWindow->Handle, 0, 1000, 0);
    
    /*Activate process monitoring*/
    NtSuspendProcess = (TYPE_NtSuspendProcess) GetProcAddress(
                                                GetModuleHandleW(L"ntdll.dll"),
                                                "NtSuspendProcess"
                                                );

    NtResumeProcess = (TYPE_NtResumeProcess) GetProcAddress(
                                                GetModuleHandleW(L"ntdll.dll"),
                                                "NtResumeProcess"
                                                );

    PushToggleProcessMonitoring(TRUE);

    g_szPrevGame[5] = '\0';

    PushMonitorThreadHandle = CreateRemoteThread(
        NtCurrentProcess(), 
        0, 
        0, 
        &MonitorThread, 
        NULL, 
        0, 
        NULL
        );


    // Handle messages

    while(GetMessageW(&messages, 0,0,0))
    {
        TranslateMessage(&messages);

        DispatchMessageW(&messages);
    }

    return 0;
}


WCHAR*
GetDirectoryFile( WCHAR *pszFileName )
{
    static WCHAR szPath[260];

    GetCurrentDirectoryW(260, szPath);

    SlStringConcatenate(szPath, L"\\");
    SlStringConcatenate(szPath, pszFileName);

    return szPath;
}


VOID
UpdateSharedMemory()
{
    /*PushSharedMemory->CpuLoad               = hardware.processor.load;
    PushSharedMemory->CpuTemp               = hardware.processor.temperature;
    PushSharedMemory->GpuLoad               = hardware.videoCard.load;
    PushSharedMemory->GpuTemp               = hardware.videoCard.temperature;
    PushSharedMemory->VramLoad              = hardware.videoCard.vram.usage;
    PushSharedMemory->VramMegabytesUsed     = hardware.videoCard.vram.megabytes_used;
    PushSharedMemory->MemoryLoad            = hardware.memory.usage;
    PushSharedMemory->MemoryMegabytesUsed   = hardware.memory.megabytes_used;
    PushSharedMemory->MaxThreadUsage        = hardware.processor.MaxThreadUsage;
    PushSharedMemory->MaxCoreUsage          = hardware.processor.MaxCoreUsage;*/

    PushSharedMemory->HarwareInformation = hardware;
}


VOID
PushOnTimer()
{
    RefreshHardwareInfo();
    UpdateSharedMemory();
}


LONG
__stdcall
TrayIconProc(VOID *hWnd,
             UINT32 message,
             UINT32 wParam,
             LONG lParam)
{
    // The option here is to maintain a list of all TrayIcon windows,
    // and iterate through them. If you do this, remove these 3 lines.
    //CSystemTray* pTrayIcon = m_pThis;
    if (g_hTrayIcon != hWnd)
    {
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    // If maintaining a list of TrayIcon windows, then the following...
    // pTrayIcon = GetFirstTrayIcon()
    // while (pTrayIcon != NULL)
    // {
    //    if (pTrayIcon->GetSafeHwnd() != hWnd) continue;

          // Taskbar has been recreated - all TrayIcons must process this.
    if (message == g_iTaskbarCreatedMsg)
    {
        return OnTaskbarCreated(wParam, lParam);
    }

    // Is the message from the icon for this TrayIcon?
    if (message == WM_ICON_NOTIFY)
    {
        return TrayIconNotification(wParam, lParam);
    }

    // Message has not been processed, so default.
    return DefWindowProcW(hWnd, message, wParam, lParam);
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