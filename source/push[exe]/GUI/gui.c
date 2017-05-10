#include <sl.h>

#include "gui.h"


HANDLE  Gui_InvisibleWindowHandle;
HANDLE  Gui_IconImageHandle;
HANDLE  Gui_TrayIconHandle;


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
#define WM_ICON_NOTIFY WM_APP+10
DWORD           Tray_MenuFlags;
//#define IDM_RESTORE 104
#define IDM_EXIT    105
typedef INT32(__stdcall *TYPE_Shell_NotifyIconW)(
    DWORD dwMessage,
    NOTIFYICONDATA *lpData
    );
HANDLE Tray_ParentWindowHandle;
HANDLE Tray_InvisibleWindowHandle;
HANDLE Tray_TrayIconHandle;

NOTIFYICONDATA  Tray_IconData;
DWORD           Tray_MenuFlags;
DWORD           Tray_TaskBarCreatedMessage;
TYPE_Shell_NotifyIconW Shell_NotifyIconW;


VOID SlTrayGetMenuHandles(VOID** Menu, VOID** hSubMenu)
{
    VOID* subMenu;

    *Menu = CreatePopupMenu();
    subMenu = CreatePopupMenu();

    AppendMenuW(*Menu, MF_POPUP, (UINT_B)subMenu, L"Menu");
    AppendMenuW(subMenu, MF_STRING, BUTTON_ADDGAME, L"Add Game");
    AppendMenuW(subMenu, MF_STRING, BUTTON_STOPRAMDISK, L"Stop RAMDisk");
    AppendMenuW(subMenu, MF_STRING, BUTTON_MANUALADD, L"Manual Add");
    AppendMenuW(subMenu, MF_STRING, MENU_OVERCLOCK, L"Overclock");
    AppendMenuW(subMenu, MF_STRING, MENU_CACHE, L"Cache");
    AppendMenuW(subMenu, MF_STRING, IDM_EXIT, L"Exit");

    *hSubMenu = GetSubMenu(*Menu, 0);
}


LONG __stdcall TrayIconProc(
    HANDLE WindowHandle,
    UINT32 message,
    UINT32 wParam,
    LONG lParam
    )
{
    if (message == WM_ICON_NOTIFY)
    {
        // Clicking with right button brings up a context menu

        if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            VOID *hMenu;
            VOID *hSubMenu;
            POINT pos;


            SlTrayGetMenuHandles(&hMenu, &hSubMenu);

            // Make chosen menu item the default (bold font)

            SetMenuDefaultItem(hSubMenu, 0, TRUE);


            // Display and track the popup menu

            GetCursorPos(&pos);

            TrackPopupMenu(
                hSubMenu,
                0,
                pos.x,
                pos.y,
                0,
                Tray_ParentWindowHandle ? Tray_ParentWindowHandle : Tray_TrayIconHandle,
                NULL
                );

            DestroyMenu(hMenu);
        }

        return -1;
    }

    if (message == Tray_TaskBarCreatedMessage)
    {
        // Reset the flags to what was used at creation
        Tray_IconData.Flags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

        // Try and recreate the icon
        Shell_NotifyIconW(NIM_ADD, &Tray_IconData);
    }

    if (message == WM_COMMAND)
    {
        if (LOWORD(wParam) == IDM_EXIT)
        {
            DestroyWindow(Tray_TrayIconHandle);
            PostQuitMessage(0);
        }
    }

    if (message == WM_DESTROY)
    {
        Tray_IconData.Flags = 0;

        Shell_NotifyIconW(NIM_DELETE, &Tray_IconData);
    }

    return DefWindowProcW(WindowHandle, message, wParam, lParam);
}

/**
* Minimizes a window to the system tray.
*
* \param ParentWindowHandle Optional; Handle to the parent window.
* This window will be set invisible.
* \param IconImage Image for the icon.
* \param IconText Text to display when mouse hovers over try icon.
* \param MenuFlags Choose what menu items to display.
* \param InvisibleWindowHandle Optional; Returns the handle of the
* created invisible window.
* \param TrayIconHandle Optional; Returns the handle of the tray icon.
*/

VOID Tray_Minimize(
    HANDLE ParentWindowHandle,
    HANDLE IconImage,
    WCHAR* IconText,
    DWORD MenuFlags,
    HANDLE* InvisibleWindowHandle,
    HANDLE* TrayIconHandle
    )
{
    WNDCLASSEX windowClass;

	Memory_Clear(&windowClass, sizeof(WNDCLASSEX));

    windowClass.Size = sizeof(WNDCLASSEX);
    windowClass.Style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    windowClass.WndProc = (VOID*)TrayIconProc;
    windowClass.ClassName = L"TrayIconClass";

    RegisterClassExW(&windowClass);

    Tray_TrayIconHandle = CreateWindowExW(
        0L,
        L"TrayIconClass",
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

    if (TrayIconHandle)
        *TrayIconHandle = Tray_TrayIconHandle;

    Memory_Clear(&Tray_IconData, sizeof(NOTIFYICONDATA));

    Tray_IconData.cbSize = sizeof(NOTIFYICONDATA);
    Tray_IconData.NotifyWindowHandle = Tray_TrayIconHandle;
    Tray_IconData.hIcon = IconImage;
    Tray_IconData.Flags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    Tray_IconData.uCallbackMessage = WM_ICON_NOTIFY;

    String_CopyN(Tray_IconData.szTip, IconText, String_GetLength(IconText));

    Shell_NotifyIconW = (TYPE_Shell_NotifyIconW)Module_GetProcedureAddress(Module_Load(L"shell32.dll"), "Shell_NotifyIconW");

    Shell_NotifyIconW(NIM_ADD, &Tray_IconData);

    Tray_MenuFlags = MenuFlags;
    Tray_TaskBarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    // Create static invisible window.

    if (ParentWindowHandle)
    {
        Tray_ParentWindowHandle = ParentWindowHandle;

        if (IsWindow(Tray_InvisibleWindowHandle) == FALSE)
        {
            Tray_InvisibleWindowHandle = CreateWindowExW(
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

        if (InvisibleWindowHandle)
            *InvisibleWindowHandle = Tray_InvisibleWindowHandle;

        SetParent(ParentWindowHandle, Tray_InvisibleWindowHandle);

        SetWindowLongW(
            ParentWindowHandle,
            GWL_STYLE,
            GetWindowLongW(ParentWindowHandle, GWL_STYLE) &~WS_VISIBLE
            );
    }
}


VOID
MaximiseFromTray( VOID *hWnd )
{
    SetParent(hWnd, NULL);

    SetWindowLongW(
        hWnd, 
        GWL_STYLE, 
        GetWindowLongW(hWnd, GWL_STYLE) | WS_VISIBLE
        );

    RedrawWindow(
        hWnd, 
        NULL, 
        NULL, 
        RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME | RDW_INVALIDATE | RDW_ERASE
        );

    // Move focus away and back again to ensure taskbar icon is recreated

    if (IsWindow(Gui_InvisibleWindowHandle))
    {
        SetActiveWindow(Gui_InvisibleWindowHandle);
    }

    SetActiveWindow(hWnd);
    SetForegroundWindow(hWnd);
    DestroyWindow(Gui_TrayIconHandle);
}


