#include <sltypes.h>
#include <slnt.h>
#include <slntuapi.h>
#include <pushbase.h>
//#include <stdio.h>
#include <string.h>
#include <slc.h>
#include <wchar.h>
#include "push.h"
#include "gui.h"


BOOLEAN g_bShowIconPending = 0;
NOTIFYICONDATA  PushIconData;
VOID*   g_hInvisibleWindow = 0;
VOID* GuiIconHandle;


INT32 __stdcall ListViewCompareProc(
    LONG lParam1,
    LONG lParam2,
    LONG lParamSort)
{
    unsigned long iBytes1   = (unsigned long) lParam1;
    unsigned long iBytes2   = (unsigned long) lParam2;
    int iResult             = iBytes1 - iBytes2;

    return(-iResult);
}


VOID InstallIconPending()
{
    // Is the icon display pending, and it's not been set as "hidden"?
    if (!g_bShowIconPending)
        return;

    // Reset the flags to what was used at creation
    PushIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

    // Try and recreate the icon
    Shell_NotifyIconW(NIM_ADD, &PushIconData);

    // If it's STILL hidden, then have another go next time...
    g_bShowIconPending = TRUE;
}


// This is called whenever the taskbar is created (eg after explorer crashes
// and restarts. Please note that the WM_TASKBARCREATED message is only passed
// to TOP LEVEL windows (like WM_QUERYNEWPALETTE)
LONG
OnTaskbarCreated( UINT32 wParam,
                  LONG lParam)
{
    InstallIconPending();

    return 0L;
}


extern "C" VOID* __stdcall CreatePopupMenu(
    VOID
    );


#define MF_STRING           0x00000000L
extern "C" INTBOOL
    __stdcall
    AppendMenuW(
    VOID* hMenu,
    UINT32 uFlags,
    UINT_B uIDNewItem,
    WCHAR* lpNewItem
    );
#define MF_POPUP            0x00000010L


VOID
GetMenuHandles( VOID** Menu, VOID** hSubMenu )
{
    VOID* subMenu;

    //*Menu = LoadMenuW(g_hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU));
    *Menu = CreatePopupMenu();
    subMenu = CreatePopupMenu();

    AppendMenuW(*Menu, MF_POPUP, (UINT_B)subMenu, L"Menu");
    AppendMenuW(subMenu, MF_STRING, IDM_RESTORE, L"Restore");
    AppendMenuW(subMenu, MF_STRING, IDM_EXIT, L"Exit");

    *hSubMenu = GetSubMenu(*Menu, 0);
}


LONG
TrayIconNotification( UINT32 wParam, LONG lParam )
{
    // Clicking with right button brings up a context menu

    if (LOWORD(lParam) == WM_RBUTTONUP)
    {
        VOID *hMenu;
        VOID *hSubMenu;
        POINT pos;


        GetMenuHandles(&hMenu, &hSubMenu);

        // Make chosen menu item the default (bold font)

        SetMenuDefaultItem(hSubMenu, 0, TRUE);


        // Display and track the popup menu

        GetCursorPos(&pos);


        SetForegroundWindow(PushMainWindow->Handle);

        TrackPopupMenu(hSubMenu, 0, pos.x, pos.y, 0, PushMainWindow->Handle, NULL);

        // BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
        PostMessageW(PushMainWindow->Handle, 0,0,0);

        DestroyMenu(hMenu);
    }
    else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
    {
        // double click received, the default action is to execute default menu item

        UINT32  iItem;
        VOID    *hMenu;
        VOID    *hSubMenu;

        SetForegroundWindow(PushMainWindow->Handle);

        GetMenuHandles(&hMenu, &hSubMenu);

        iItem = GetMenuItemID(hSubMenu, 0);

        DestroyMenu(hMenu);

        PostMessageW(PushMainWindow->Handle, WM_COMMAND, iItem, 0);
    }

    return -1;
}

VOID* __stdcall LoadImageW(
    VOID* hinst,
    WCHAR* lpszName,
    UINT32 uType,
    int cxDesired,
    int cyDesired,
    UINT32 fuLoad
    );
#define IMAGE_ICON          1
#define LR_LOADFROMFILE     0x00000010
#define LR_DEFAULTSIZE      0x00000040
#define LR_SHARED           0x00008000


VOID
MinimiseToTray( VOID *hWnd )
{
    BOOLEAN bResult = TRUE;
    WNDCLASSEX windowClass = { 0 };

    windowClass.Size = sizeof(WNDCLASSEX);
    windowClass.Style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    windowClass.WndProc = (VOID*) TrayIconProc;
    windowClass.Instance = PushInstance;
    windowClass.ClassName = L"TrayIconClass";

    RegisterClassExW( &windowClass );

    g_hTrayIcon = CreateWindowExW(
                    0L,
                    L"TrayIconClass",
                    L"",
                    WS_POPUP,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL, NULL,
                    PushInstance,
                    NULL
                    );

    memset(&PushIconData, 0, sizeof(NOTIFYICONDATA));

    PushIconData.cbSize = sizeof(NOTIFYICONDATA);
    PushIconData.hWnd = PushMainWindow->Handle;
    PushIconData.uID = IDR_POPUP_MENU;
    PushIconData.hIcon = GuiIconHandle;
    PushIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    PushIconData.uCallbackMessage = WM_ICON_NOTIFY;

    SlStringCopy(PushIconData.szTip, L"Push", 64);

    bResult = Shell_NotifyIconW(NIM_ADD, &PushIconData);

    g_bShowIconPending = !bResult;

    g_iTaskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");

    // Create static invisible window

    if (!IsWindow(g_hInvisibleWindow))
    {
        g_hInvisibleWindow = CreateWindowExW(
                                0,
                                L"Static",
                                L"",
                                WS_POPUP,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );
    }


    SetParent(hWnd, g_hInvisibleWindow);

    SetWindowLongW(
        hWnd,
        GWL_STYLE,
        GetWindowLongW(hWnd, GWL_STYLE) &~ WS_VISIBLE
        );
}


VOID
MaximiseFromTray( VOID *hWnd )
{
    SetParent(hWnd, NULL);

    SetWindowLongW(hWnd, GWL_STYLE, GetWindowLongW(hWnd, GWL_STYLE) | WS_VISIBLE);

    RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME |
                       RDW_INVALIDATE | RDW_ERASE);

    // Move focus away and back again to ensure taskbar icon is recreated

    if (IsWindow(g_hInvisibleWindow))
    {
        SetActiveWindow(g_hInvisibleWindow);
    }

    SetActiveWindow(hWnd);

    SetForegroundWindow(hWnd);

    g_bShowIconPending = FALSE;

    PushIconData.uFlags = 0;

    Shell_NotifyIconW(NIM_DELETE, &PushIconData);

    DestroyWindow(g_hTrayIcon);
}
