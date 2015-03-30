#include <sl.h>
#include <sal.h>
#include <sldriver.h>
#include <stdio.h>
#include <string.h>

#include "push.h"
#include "ring0.h"


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
    BYTE*   lpReserved2;
    HANDLE  hStdInput;
    HANDLE  hStdOutput;
    HANDLE  hStdError;
} STARTUPINFO, *LPSTARTUPINFOW;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;


INTBOOL __stdcall IsWow64Process(
    HANDLE hProcess,
    INTBOOL* Wow64Process
    );

UINT32 __stdcall GetDriveTypeW(
    WCHAR* lpRootPathName
    );

DWORD __stdcall GetTempPathW(
    DWORD nBufferLength,
    WCHAR* lpBuffer
    );

INTBOOL __stdcall Wow64DisableWow64FsRedirection(
    VOID **OldValue
    );

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


#define DRIVE_REMOTE      4

#define STATUS_INVALID_IMAGE_HASH       ((NTSTATUS)0xC0000428L)
#define STATUS_DRIVER_BLOCKED_CRITICAL  ((NTSTATUS)0xC000036BL)
#define MB_ICONQUESTION 0x00000020L
#define MB_YESNO        0x00000004L

#define IDYES 6

#define STARTF_USESHOWWINDOW    0x00000001
#define SW_SHOWMINIMIZED    2


VOID Driver_Extract()
{
    INTBOOL isWow64 = FALSE;

    IsWow64Process(NtCurrentProcess(), &isWow64);

    if (isWow64)
        ExtractResource(L"DRIVER64", L"push0.sys");
    else
        ExtractResource(L"DRIVER32", L"push0.sys");
}


VOID Driver_Load()
{
    WCHAR dir[260];
    WCHAR *ptr;
    WCHAR driverPath[260];
    INT32 msgId;
    NTSTATUS status;

    GetModuleFileNameW(NULL, dir, 260);
    if ((ptr = SlStringFindLastChar(dir, '\\')) != NULL)
    {
        *ptr = '\0';
    }
    swprintf(driverPath, 260, L"%s\\%s", dir, L"push0.sys");

    WCHAR root[4];
    root[0] = driverPath[0];
    root[1] = ':';
    root[2] = '\\';
    root[3] = '\0';

    if (root[0] == '\\' || GetDriveTypeW((WCHAR*)root) == DRIVE_REMOTE)
    {
        WCHAR tempPath[260];

        GetTempPathW(260, tempPath);
        SlStringConcatenate(tempPath, L"push0.sys");
        SlFileCopy(driverPath, tempPath, NULL);
        SlStringCopy(driverPath, tempPath);
    }

    status = SlLoadDriver(
        L"PUSH",
        L"push0.sys",
        L"Push Kernel-Mode Driver",
        L"\\\\.\\Push",
        TRUE,
        &R0DriverHandle
        );

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            MessageBoxW(NULL, L"Driver file not found!", L"Error", 0);
        }

        if (status == STATUS_DRIVER_BLOCKED_CRITICAL)
        {
            // Probably wrong driver. Overwrite.

            DeleteFileW(driverPath);
            Driver_Extract();
            
            // Try again.
            Driver_Load();
        }

        if (status == STATUS_INVALID_IMAGE_HASH)
        {
            //prompt user to enable test-signing.
            msgId = MessageBoxW(
                NULL,
                L"The driver failed to load because it isn't signed. "
                L"It is required for Push to work correctly. "
                L"Do you want to enable test-signing to be able to use driver functions?",
                L"Driver Signing",
                MB_ICONQUESTION | MB_YESNO
                );

            if (msgId == IDYES)
            {
                STARTUPINFO startupInfo = { 0 };
                PROCESS_INFORMATION processInfo = { 0 };
                BOOLEAN status;
                DWORD error = NULL;
                VOID* oldValue = NULL;

                Wow64DisableWow64FsRedirection(&oldValue);

                startupInfo.cb = sizeof(startupInfo);
                startupInfo.dwFlags = STARTF_USESHOWWINDOW;
                startupInfo.wShowWindow = SW_SHOWMINIMIZED;

                status = CreateProcessW(
                    L"C:\\Windows\\System32\\bcdedit.exe",
                    L"\"C:\\Windows\\System32\\bcdedit.exe\" /set TESTSIGNING ON",
                    NULL,
                    NULL,
                    FALSE,
                    NULL,
                    NULL,
                    L"C:\\Windows\\System32",
                    &startupInfo,
                    &processInfo
                    );

                MessageBoxW(NULL, L"Restart your computer to load driver", L"Restart required", NULL);
            }
        }
    }
}