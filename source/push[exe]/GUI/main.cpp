#include <sltypes.h>
#include <slntuapi.h>
#include <slgui.h>
#include <slc.h>
#include <wchar.h>
#include <ramdisk.h>
#include <ini.h>
#include <file.h>
#include <pushbase.h>
#include <ring0.h>

#include "gui.h"
#include "main.h"
#include "cache.h"
#include "copy.h"


WINDOW tab;
WINDOW* PushMainWindow;
WINDOW* cacheWindow;
WINDOW fake;

VOID*   g_hTrayIcon = 0;
DWORD   g_iTaskbarCreatedMsg = 0;
BfBatchFile* MwBatchFile;
VOID* MwControlHandles[50];
FILE_LIST MwFileList;


CONTROL MwOsdControls[] = {
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_GPU_LOAD,      L"GPU Core utilization"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_GPU_TEMP,      L"GPU Core temperature"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_GPU_E_CLK,     L"GPU Engine Clock"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_GPU_M_CLK,     L"GPU Memory Clock"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_GPU_VRAM,      L"GPU VRAM usage"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_CPU_LOAD,      L"CPU utilization"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_CPU_TEMP,      L"CPU temperature"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_RAM,           L"RAM usage"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_MCU,           L"Max core usage"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_MTU,           L"Max thread usage"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_DISK_RWRATE,   L"Disk read-write rate"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_DISK_RESPONSE, L"Disk response time"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_BACKBUFFERS,   L"Frame Buffer count"},
    {L"Button", BS_AUTOCHECKBOX,    CHECKBOX_TIME,          L"Show Time"},
    {L"Button", NULL,               BUTTON_RESET_GPU,       L"Reset overloads", 285,    190},
    {L"Button", NULL,               BUTTON_TRAY,            L"To tray..",       NULL,   190}
};

CONTROL MwCacheControls[] = {
    {L"Button",     NULL,           BUTTON_STOPRAMDISK, L"Stop RAMDisk",    NULL, 190},
    {L"Button",     NULL,           BUTTON_ADDGAME,     L"Add Game",        NULL, 190},
    {L"ComboBox",   CBS_DROPDOWN,   COMBOBOX_GAMES,     NULL,               NULL, 190}
};


typedef struct _TCITEM
{
    UINT32 Mask;
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR* pszText;
    INT32   cchTextMax;
    INT32 iImage;
    UINT_B lParam;
} TCITEM;
#define TCIF_TEXT               0x0001
#define TCIF_IMAGE              0x0002
//#define SNDMSG SendMessageW
#define TCM_FIRST               0x1300      // Tab control messages
#define TCM_INSERTITEM        (TCM_FIRST + 62)
typedef NMHDR *LPNMHDR;
#define TCN_FIRST               (0U-550U)       // tab control
#define TCN_SELCHANGE           (TCN_FIRST - 1)
#define TabCtrl_InsertItem(hwnd, iItem, pitem)   \
    (int)SendMessageW((hwnd), TCM_INSERTITEM, (UINT_B)(int)(iItem), (UINT_B)(const TCITEM *)(pitem))


#define TCM_GETCURSEL           (TCM_FIRST + 11)
#define TabCtrl_GetCurSel(hwnd) \
    (int)SendMessageW((hwnd), TCM_GETCURSEL, 0, 0)


VOID* PushVisiblePage;
VOID* PushTabPages[2];
typedef UINT_B (__stdcall *LPOFNHOOKPROC) (VOID*, UINT32, UINT_B, UINT_B);
typedef struct _OPENFILENAME
{

  DWORD         lStructSize;
  VOID*         hwndOwner;
  VOID*         hInstance;
  WCHAR*        lpstrFilter;
  WCHAR*        lpstrCustomFilter;
  DWORD         nMaxCustFilter;
  DWORD         nFilterIndex;
  WCHAR*        lpstrFile;
  DWORD         nMaxFile;
  WCHAR*        lpstrFileTitle;
  DWORD         nMaxFileTitle;
  WCHAR*        lpstrInitialDir;
  WCHAR*        Title;
  DWORD         Flags;
  WORD          nFileOffset;
  WORD          nFileExtension;
  WCHAR*        lpstrDefExt;
  UINT_B  lCustData;
  LPOFNHOOKPROC lpfnHook;
  WCHAR*        lpTemplateName;
  VOID*         pvReserved;
  DWORD         dwReserved;
  DWORD         FlagsEx;

} OPENFILENAME;


#define OFN_EXPLORER                 0x00080000     // new look commdlg
#define OFN_FILEMUSTEXIST            0x00001000
#define OFN_HIDEREADONLY             0x00000004
#define OFN_NOCHANGEDIR              0x00000008

BOOLEAN TabCtrl_OnSelChanged(VOID)
{
    int iSel = TabCtrl_GetCurSel(tab.Handle);

    //Hide the current child dialog box, if any.
    ShowWindow(PushVisiblePage, FALSE);

    //ShowWindow() does not seem to post the WM_SHOWWINDOW message
    // to the tab page.  Since I use the hiding of the window as an
    // indication to stop the message loop I post it explicitly.
    //PostMessage() is necessary in the event that the Loop was started
    // in response to a mouse click.
    //FORWARD_WM_SHOWWINDOW(m_lptc->hVisiblePage,FALSE,0,PostMessage);

    //Show the new child dialog box.
    ShowWindow(PushTabPages[iSel], TRUE);

    // Save the current child
    PushVisiblePage = PushTabPages[iSel];

    return TRUE;
}


VOID
GetPathOnly( WCHAR *pszFilePath, WCHAR *pszBuffer )
{
    WCHAR *pszLastSlash;


    pszLastSlash = SlStringFindLastChar(pszFilePath, '\\');

    SlStringCopy(pszBuffer,
            pszFilePath,
            (pszLastSlash - pszFilePath) + 1);

    *(pszBuffer + (pszLastSlash - pszFilePath) + 1) = '\0';
}


VOID
ListViewAddItems()
{
    FILE_LIST_ENTRY *file = MwFileList;
    WCHAR text[260];
    UINT16 item;

    while (file != NULL)
    {
        item = CwListView->AddItem(file->Name, file->Bytes);

        swprintf(text, 260, L"%u", file->Bytes);
        CwListView->SetItem(text, item, 1);

        if (file->Cache)
            CwListView->SetItemState(item, TRUE);

        file = file->NextEntry;
    }
}


WCHAR*
GetComboBoxData()
{
    INT32 iIndex;

    iIndex = SendMessageW(
                MwControlHandles[COMBOBOX_GAMES],
                CB_GETCURSEL,
                0,
                0
                );

    return (WCHAR*) SendMessageW(
                        MwControlHandles[COMBOBOX_GAMES],
                        CB_GETITEMDATA,
                        iIndex,
                        0
                        );
}


VOID
CreateControl(
    WINDOW* window,
    CONTROL* Control
    )
{
    VOID *handle;

    if (Control->VerticalPos != 0)
        window->lastPos = Control->VerticalPos;

    if (!Control->Width)
        Control->Width = 200;

    if (!Control->Height)
        Control->Height = 20;

    handle = CreateWindowExW(
                0L,
                Control->Class,
                Control->Name,
                WS_CHILD | WS_VISIBLE | Control->Type,
                0,
                window->lastPos,
                Control->Width,
                Control->Height,
                window->Handle,
                (VOID *) Control->Id,
                0,
                0
                );

    window->lastPos += 20;
    MwControlHandles[Control->Id] = handle;
}

VOID
AppendFileNameToPath(
    WCHAR* FileName,
    WCHAR* Path,
    WCHAR* Buffer
    )
{
    wcscpy(Buffer, Path);

    wcscat(Buffer, L"\\");
    wcscat(Buffer, FileName);
}


VOID
BuildFileList(
    WCHAR* Directory,
    FILE_DIRECTORY_INFORMATION* Information,
    FS_ENUM_DIRECTORY Callback
    )
{
    WCHAR fileName[260];
    UINT32 fileNameLength;

    // Skip filenames that start with "." and ".."
    if (Information->FileName[0] == '.')
        return;

    fileNameLength = Information->FileNameLength / sizeof(WCHAR);

    wcsncpy(fileName, Information->FileName, fileNameLength);

    // Add terminating null
    fileName[fileNameLength] = 0;

    if (Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        WCHAR directory[260];

        AppendFileNameToPath(fileName, Directory, directory);
        FsEnumDirectory(directory, L"*", Callback);
    }
    else
    {
        FILE_LIST_ENTRY fileEntry;
        WCHAR *filePath, slash[] = L"\\";

        filePath = (WCHAR*) RtlAllocateHeap(
            PushHeapHandle,
            0,
            (wcslen(Directory) * sizeof(WCHAR)) + sizeof(slash) + Information->FileNameLength
            );

        AppendFileNameToPath(fileName, Directory, filePath);

        fileEntry.Bytes = Information->EndOfFile.u.LowPart;
        fileEntry.Name = filePath;
        fileEntry.Cache = MwBatchFile->IsBatchedFile(&fileEntry);

        PushAddToFileList(&MwFileList, &fileEntry);
        RtlFreeHeap(PushHeapHandle, 0, filePath);
    }
}


VOID
BuildGameFilesList(
    WCHAR* Directory,
    FILE_DIRECTORY_INFORMATION* Information
    )
{
    BuildFileList(
        Directory,
        Information,
        BuildGameFilesList
        );
}


VOID
MwCreateMainWindow()
{
    WINDOW pageWindow;
    WINDOW cachePage;

    RECT windowDimensions;
    void* page;
    TCITEM tabItem;
    WNDCLASSEX wc;
    INT32 i = 0, pageTopOffset = 24;

    //initialize icon handle
    GuiIconHandle = LoadIconW(PushInstance, L"PUSH_ICON");



    // Create Window
    PushMainWindow = (WINDOW*) RtlAllocateHeap(
                                PushHeapHandle,
                                0,
                                sizeof(WINDOW)
                                );

    PushMainWindow->lastPos = 0;
    PushMainWindow->Handle = SlCreateWindow(
                                0,
                                L"RTSSPush",
                                L"Push",
                                200,
                                383,
                                MainWndProc,
                                0,
                                GuiIconHandle
                                );

    // Get dimensions of parent window
    GetClientRect(PushMainWindow->Handle, &windowDimensions);

    //Create tab control
    tab.Handle = CreateWindowExW(
                    NULL,
                    L"SysTabControl32",
                    L"",
                    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                    0, 0,
                    windowDimensions.right,
                    windowDimensions.bottom,
                    PushMainWindow->Handle,
                    NULL,
                    PushInstance,
                    NULL
                    );

    // Insert Tabs
    tabItem.Mask = TCIF_TEXT | TCIF_IMAGE;
    tabItem.iImage = -1;
    tabItem.pszText = L"OSD";

    TabCtrl_InsertItem(tab.Handle, 0, &tabItem);

    tabItem.pszText = L"Cache";
    TabCtrl_InsertItem(tab.Handle, 1, &tabItem);
    GetClientRect(tab.Handle, &windowDimensions);

    wc.Size        = sizeof(WNDCLASSEX);
    wc.Style         = 0;
    wc.WndProc   = MainWndProc;
    wc.ClsExtra    = 0;
    wc.WndExtra    = 0;
    wc.Instance     = PushInstance;
    wc.Icon         = NULL;
    wc.Cursor       = NULL;
    wc.Background = NULL;
    wc.MenuName  = NULL;
    wc.ClassName = L"Page";
    wc.IconSm       = NULL;

    RegisterClassExW(&wc);

    page = CreateWindowExW(0,L"Page", L"",
        WS_CHILD | WS_VISIBLE,
        windowDimensions.top,
        windowDimensions.left + pageTopOffset,
        windowDimensions.right,
        windowDimensions.bottom,
        PushMainWindow->Handle, NULL, PushInstance,
        NULL
        );

    pageWindow.Handle = page;
    pageWindow.lastPos = 0;
    PushVisiblePage = page;
    PushTabPages[0] = page;

    page = CreateWindowExW(
            0,
            L"Page", L"",
        WS_CHILD,
        windowDimensions.top,
        windowDimensions.left + pageTopOffset,
        windowDimensions.right,
        windowDimensions.bottom,
        PushMainWindow->Handle,
        NULL,
        PushInstance,
        NULL
        );

    PushTabPages[1] = page;
    cachePage.lastPos = 0;
    cachePage.Handle = page;


    tab.lastPos = 0;
    INT32 iControls = sizeof(MwOsdControls) / sizeof(MwOsdControls[0]);
    for (i = 0; i < iControls; i++)
    {
        CreateControl( &pageWindow, &MwOsdControls[i] );
    }

    iControls = sizeof(MwCacheControls) / sizeof(MwCacheControls[0]);
    //cache
    for (i = 0; i < iControls; i++)
    {
        CreateControl( &cachePage, &MwCacheControls[i] );
    }
}


INT32 __stdcall MainWndProc( VOID *hWnd,UINT32 uMessage, UINT32 wParam, LONG lParam )
{
    switch (uMessage)
    {
    case WM_ICON_NOTIFY:
            return TrayIconNotification(wParam, lParam);
            break;
    case WM_TIMER:
            PushOnTimer();
            break;

    case WM_NOTIFY:
        {
            if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
            {
                TabCtrl_OnSelChanged();
            }

        }break;

    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDM_RESTORE:
                MaximiseFromTray(PushMainWindow->Handle);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
                // OSD
            case CHECKBOX_TIME:
                PushSharedMemory->OSDFlags |= OSD_TIME;
                break;
            case CHECKBOX_GPU_LOAD:
                PushSharedMemory->OSDFlags |= OSD_GPU_LOAD;
                break;
            case CHECKBOX_GPU_TEMP:
                PushSharedMemory->OSDFlags |= OSD_GPU_TEMP;
                break;
            case CHECKBOX_GPU_E_CLK:
                PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
                break;
            case CHECKBOX_GPU_M_CLK:
                PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
                break;
            case CHECKBOX_GPU_VRAM:
                PushSharedMemory->OSDFlags |= OSD_GPU_VRAM;
                break;
            case CHECKBOX_CPU_LOAD:
                PushSharedMemory->OSDFlags |= OSD_CPU_LOAD;
                break;
            case CHECKBOX_CPU_TEMP:
                PushSharedMemory->OSDFlags |= OSD_CPU_TEMP;
                break;
            case CHECKBOX_RAM:
                PushSharedMemory->OSDFlags |= OSD_RAM;
                break;
            case CHECKBOX_MCU:
                PushSharedMemory->OSDFlags |= OSD_MCU;
                break;
            case CHECKBOX_MTU:
                PushSharedMemory->OSDFlags |= OSD_MTU;
                break;
            case CHECKBOX_DISK_RWRATE:
                PushSharedMemory->OSDFlags |= OSD_DISK_RWRATE;
                break;
            case CHECKBOX_DISK_RESPONSE:
                PushSharedMemory->OSDFlags |= OSD_DISK_RESPONSE;
                break;
            case CHECKBOX_BACKBUFFERS:
                PushSharedMemory->OSDFlags |= OSD_BUFFERS;
                break;
            case BUTTON_RESET_GPU:
                PushSharedMemory->Overloads = 0;
                break;
            case BUTTON_TRAY:
                MinimiseToTray(PushMainWindow->Handle);
                break;
            case BUTTON_STOPRAMDISK:
                RemoveRamDisk();
                break;

            case BUTTON_ADDGAME:
                {
                    WCHAR filepath[260] = L"", path[260], *imageName, *slash;
                    OPENFILENAME ofn = { 0 };

                    //OpenFileGetPath(filepath);
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.lpstrFilter = L"Executable (.EXE)\0*.exe\0";
                    ofn.lpstrFile = filepath;
                    ofn.nMaxFile = 260;
                    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
                    ofn.lpstrDefExt = L"";
                    ofn.Title = L"Select game executable";

                    GetOpenFileNameW( &ofn );

                    imageName = SlStringFindLastChar(filepath, '\\') + 1;

                    if (IniReadBoolean(L"Games", imageName, FALSE))
                    {
                        imageName = filepath;
                    }

                    IniWriteBoolean(L"Games", filepath, TRUE);

                    IniWriteSubKey(L"Game Settings",
                                   filepath,
                                   L"Name",
                                   imageName);

                    GetPathOnly(filepath, path);

                    slash = SlStringFindLastChar(path, '\\');

                    *slash = '\0';

                    IniWriteSubKey(L"Game Settings",
                                   filepath,
                                   L"Path",
                                   path);

                    IniWriteSubKey(L"Game Settings", filepath, L"UseRamDisk", L"True");

                } break;

            case COMBOBOX_GAMES:
                {
                    if (HIWORD(wParam) == CBN_DROPDOWN)
                    {
                        static BOOLEAN gamesInited = FALSE;

                        if (!gamesInited)
                        {
                            WCHAR *pszGames;
                            INT32   i;

                            pszGames = IniReadString(L"Games", 0,0);

                            if (pszGames)
                            {
                                for (i = 0; pszGames[0] != '\0'; i++)
                                {
                                    SendMessageW(
                                        MwControlHandles[COMBOBOX_GAMES],
                                        CB_ADDSTRING,
                                        0,
                                        (LONG) IniReadSubKey(L"Game Settings", pszGames, L"Name")
                                        );

                                    SendMessageW(
                                        MwControlHandles[COMBOBOX_GAMES],
                                        CB_SETITEMDATA,
                                        i,
                                        (LONG) pszGames
                                        );

                                    pszGames = SlStringFindLastChar(pszGames, '\0') + 1;
                                }
                            }

                            gamesInited = TRUE;
                        }

                    }

                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        static INT32 iControls = sizeof(MwCacheControls) / sizeof(MwCacheControls[0]);
                        INT32 j = 0;

                        cacheWindow = (WINDOW *) RtlAllocateHeap(PushHeapHandle, 0, sizeof(WINDOW));

                        cacheWindow->lastPos = 0;

                        cacheWindow->Handle = SlCreateWindow(
                                                0,
                                                L"Cache Manager",
                                                L"Cache Settings",
                                                600,
                                                500,
                                                CacheWndProc,
                                                PushMainWindow->Handle,
                                                NULL
                                                );
                    }

                } break;
            }

        } break;

    case WM_DESTROY:
        {
            PushToggleProcessMonitoring(FALSE);

            PostQuitMessage(0);

        } break;
    }

    return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}
