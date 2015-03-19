// Window Class

typedef struct _WNDCLASSEXW {
    UINT32  Size;
    UINT32  Style;
    VOID*   WndProc;
    INT32   ClsExtra;
    INT32   WndExtra;
    VOID*   Instance;
    VOID*   Icon;
    VOID*   Cursor;
    VOID*   Background;
    WCHAR*  MenuName;
    WCHAR*  ClassName;
    VOID*   IconSm;
} WNDCLASSEX;

// Window Field Offsets

#define GWL_WNDPROC (-4)


// Window Messages

#define WM_CREATE           0x0001
#define WM_DESTROY          0x0002
#define WM_CLOSE            0x0010
#define WM_SETTINGCHANGE    0x001A
#define WM_NOTIFY           0x004E
#define WM_COMMAND          0x0111
#define WM_TIMER            0x0113
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONUP        0x0205
#define WM_APP              0x8000


// Window Styles

#define WS_CLIPSIBLINGS 0x04000000L

// Button Control Styles

#define BS_AUTOCHECKBOX 0x00000003L
#define BS_RADIOBUTTON  0x00000004L
#define BS_GROUPBOX     0x00000007L

// Combo Box Notification Codes

#define CBN_SELCHANGE   1
#define CBN_DROPDOWN    7

#define MF_POPUP            0x00000010L
#define MF_STRING           0x00000000L


INTBOOL __stdcall AppendMenuW(
    HANDLE hMenu,
    UINT32 uFlags,
    ULONG_PTR uIDNewItem,
    WCHAR* lpNewItem
    );

HANDLE __stdcall CreatePopupMenu(
    VOID
    );
