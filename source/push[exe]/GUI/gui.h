#include "push.h"


enum CONTROLS
{
    GROUPBOX_CUSTOM,
    CHECKBOX_TIME,
    CHECKBOX_GPU_LOAD,
    CHECKBOX_GPU_TEMP,
    CHECKBOX_GPU_E_CLK,
    CHECKBOX_GPU_M_CLK,
    CHECKBOX_GPU_VRAM,
    CHECKBOX_CPU_LOAD,
    CHECKBOX_CPU_TEMP,
    CHECKBOX_RAM,
    CHECKBOX_MCU,
    CHECKBOX_MTU,
    CHECKBOX_DISK_RWRATE,
    CHECKBOX_DISK_RESPONSE,
    CHECKBOX_BACKBUFFERS,
    BUTTON_CACHE,
    BUTTON_RESET_GPU,
    BUTTON_STOPRAMDISK,
    BUTTON_EDITCACHE,
    BUTTON_TRAY,
    COMBOBOX_GAMES,
    BUTTON_ADDGAME
};


#define IDM_RESTORE                       104
#define IDM_EXIT                        105
#define IDI_MAIN_ICON                 107
#define IDR_POPUP_MENU                  130



typedef struct _WINDOW{
    int     lastPos;
    void    *Handle;
} WINDOW;


typedef struct __CONTROL{

    WCHAR*  Class;
    UINT8   Type;
    UINT8   Id;
    WCHAR*  Name;
    UINT16  VerticalPos;
    UINT16  Width;
    UINT16  Height;

} CONTROL;


#ifdef __cplusplus
extern "C" WINDOW *PushMainWindow;
#else
extern WINDOW *PushMainWindow;
#endif

extern HANDLE Gui_IconImageHandle;
extern HANDLE Gui_TrayIconHandle;
extern HANDLE Gui_InvisibleWindowHandle;


LONG __stdcall MainWndProc(
    VOID* hWnd,
    UINT32 uMessage,
    UINT32 uParam,
    LONG lParam);

LONG __stdcall CacheWndProc(
    VOID*   hWnd,
    UINT32  uMessage,
    DWORD   uParam,
    LONG    lParam );



VOID
UpdateListViewOld(
    VOID*               hListView,
    FILE_LIST_ENTRY*    fileInfo
    );
#ifdef __cplusplus
extern "C" {
#endif

VOID SetListViewItemState(
     UINT32 iItem,
     BOOLEAN state
     );



 VOID ListViewAddItems();

 WCHAR* GetComboBoxData();

VOID GuiCreate();

#ifdef __cplusplus
}
#endif


VOID MaximiseFromTray (
    VOID* hWnd
    );
    
INT32 __stdcall ListViewCompareProc(
    LONG lParam1,
    LONG lParam2,
    LONG lParamSort
    );
    
VOID CreateControl(
    WINDOW* window,
    CONTROL* Control
    );


#include "batch.h"
#include "main.h"
#include "copy.h"
#include "cache.h"












