#include <sltypes.h>
#include <slntuapi.h>
#include <slgui.h>
#include <wchar.h>

#define WS_EX_TOPMOST 0x00000008L


VOID* CpwWindowHandle;
VOID* CpwTextBoxHandle;








INT32 __stdcall CopyProgessWndProc(VOID *hWnd,UINT32 uMessage, UINT32 wParam, LONG lParam)
{
    return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}


DWORD __stdcall CpwThread( VOID* Paramter )
{
    //create window
    CpwWindowHandle = SlCreateWindow(
                        WS_EX_TOPMOST,
                        L"Copy Progress",
                        L"Copy Progress",
                        400,
                        200,
                        CopyProgessWndProc,
                        NULL,
                        NULL
                        );

    CpwTextBoxHandle = CreateWindowExW(
                        0,
                        L"static",
                        L"ST_U",
                        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                        100,
                        100,
                        200,
                        13,
                        CpwWindowHandle,
                        NULL,
                        NULL,
                        NULL
                        );

    SetForegroundWindow(CpwWindowHandle);
    SlHandleMessages();

    return 0;
}
