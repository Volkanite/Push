#include <sltypes.h>
#include <slnt.h>
#include <slntuapi.h>
#include <pushbase.h>
#include <string.h>
#include <slc.h>
#include <wchar.h>
#include "push.h"
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
