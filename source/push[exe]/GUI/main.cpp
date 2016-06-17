#include <sl.h>
#include <sltray.h>
#include <stdio.h>
#include <string.h>
#include <file.h>
#include <ramdisk.h>
#include <sldirectory.h>

#include "gui.h"


WINDOW tab;
WINDOW* PushMainWindow;
WINDOW* cacheWindow;
WINDOW fake;

//BfBatchFile* MwBatchFile;
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
    {L"Button", NULL,               BUTTON_RESET_GPU,       L"Reset overloads", 285,    180},
    {L"Button", NULL,               BUTTON_TRAY,            L"To tray..",       NULL,   180}
};

CONTROL MwCacheControls[] = {
    {L"Button",     NULL,           BUTTON_STOPRAMDISK, L"Stop RAMDisk",    NULL, 180},
    {L"Button",     NULL,           BUTTON_ADDGAME,     L"Add Game",        NULL, 180},
    {L"ComboBox",   CBS_DROPDOWN,   COMBOBOX_GAMES,     NULL,               NULL, 180},
    {L"Button",     NULL,           BUTTON_MANUALADD,   L"Manual Add",      100, 100}
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


VOID GetPathOnly( WCHAR *pszFilePath, WCHAR *pszBuffer )
{
    WCHAR *pszLastSlash;


    pszLastSlash = String_FindLastChar(pszFilePath, '\\');

    String_CopyN(pszBuffer,
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
        item = ListView_AddItem(file->Name, file->Bytes);

        swprintf(text, 260, L"%u", file->Bytes);
        ListView_SetItemText(text, item, 1);

        if (file->Cache)
            ListView_SetItemState(item, TRUE);

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


VOID AppendFileNameToPath(
    WCHAR* FileName,
    WCHAR* Path,
    WCHAR* Buffer
    )
{
    String_Copy(Buffer, Path);

    String_Concatenate(Buffer, L"\\");
    String_Concatenate(Buffer, FileName);
}


VOID BuildFileList(
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
        Directory_Enum(directory, L"*", Callback);
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
        fileEntry.Cache = BatchFile_IsBatchedFile(&fileEntry);

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
    Gui_IconImageHandle = LoadIconW(PushInstance, L"PUSH_ICON");

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
                                388,
                                MainWndProc,
                                0,
                                Gui_IconImageHandle
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

typedef int (__stdcall * BFFCALLBACK)(HANDLE hwnd, UINT32 uMsg, LONG lParam, LONG lpData);
typedef struct _SHITEMID        // mkid
{
    WORD      cb;             // Size of the ID (including cb itself)
    BYTE        abID[1];        // The item ID (variable length)
} SHITEMID;

typedef struct _ITEMIDLIST {
    SHITEMID mkid;
} ITEMIDLIST, *LPITEMIDLIST, *PCIDLIST_ABSOLUTE, *PIDLIST_ABSOLUTE;

typedef struct _browseinfo {
    HANDLE            hwndOwner;
    PCIDLIST_ABSOLUTE pidlRoot;
    WCHAR*            pszDisplayName;
    WCHAR*            lpszTitle;
    UINT32              ulFlags;
    BFFCALLBACK       lpfn;
    LONG            lParam;
    int               iImage;
} BROWSEINFOW, *PBROWSEINFO, *LPBROWSEINFOW;


#define BIF_EDITBOX 16
#define BIF_NEWDIALOGSTYLE 64
#define BIF_USENEWUI            (BIF_NEWDIALOGSTYLE | BIF_EDITBOX)

typedef PIDLIST_ABSOLUTE (__stdcall *TYPE_SHBrowseForFolderW)(
    _In_ LPBROWSEINFOW lpbi
    );

typedef INTBOOL (__stdcall *TYPE_SHGetPathFromIDListW)(
    _In_  PCIDLIST_ABSOLUTE pidl,
    _Out_ WCHAR*            pszPath
    );


TYPE_SHBrowseForFolderW     SHBrowseForFolderW;
TYPE_SHGetPathFromIDListW   SHGetPathFromIDListW;


    DWORD __stdcall MapFileAndCheckSumW(
        WCHAR* Filename,
        DWORD* HeaderSum,
        DWORD* CheckSum
        );

    LONG __stdcall OleInitialize(
        _In_ VOID* pvReserved
        );

    void __stdcall OleUninitialize(void);

    void __stdcall CoTaskMemFree(
        _In_opt_ VOID* pv
        );

    INT32 __stdcall DialogBoxParamW(
        _In_opt_ HANDLE hInstance,
        _In_     WCHAR*   lpTemplateName,
        _In_opt_ HANDLE      hWndParent,
        _In_opt_ VOID*   lpDialogFunc,
        _In_     LONG    dwInitParam
        );

    void __stdcall ILFree(
        _In_ VOID* pidl
        );

WCHAR* ManualLoad;


VOID OpenCacheWindow()
{
    static INT32 iControls = sizeof(MwCacheControls) / sizeof(MwCacheControls[0]);
    INT32 j = 0;

    cacheWindow = (WINDOW *)RtlAllocateHeap(PushHeapHandle, 0, sizeof(WINDOW));

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
typedef INT32 (__stdcall *TYPE_GetOpenFileNameW)( VOID* );
TYPE_GetOpenFileNameW GetOpenFileNameW;
INT32 __stdcall MainWndProc( VOID *hWnd,UINT32 uMessage, UINT32 wParam, LONG lParam )
{
    switch (uMessage)
    {
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
            {
                DestroyWindow(Gui_TrayIconHandle);
                DestroyWindow(hWnd);
            }
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
            {
                Tray_Minimize(
                    PushMainWindow->Handle,
                    Gui_IconImageHandle,
                    L"Push",
                    IDM_RESTORE,
                    &Gui_InvisibleWindowHandle,
                    &Gui_TrayIconHandle
                    );
            }
            break;

            case BUTTON_STOPRAMDISK:
                RemoveRamDisk();
                break;

            case BUTTON_ADDGAME:
                {
                    WCHAR filePath[260] = L"", path[260], *imageName, *slash, *games;
                    OPENFILENAMEW ofn = { 0 };
                    UINT8 i = 0;
                    WCHAR indexString[10];
                    PUSH_GAME game;
                    DWORD headerSum;
                    DWORD checkSum;

                    ofn.lStructSize = sizeof(OPENFILENAMEW);
                    ofn.lpstrFilter = L"Executable (.EXE)\0*.exe\0";
                    ofn.lpstrFile = filePath;
                    ofn.nMaxFile = 260;
                    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
                    ofn.lpstrDefExt = L"";
                    ofn.Title = L"Select game executable";

                    GetOpenFileNameW = (TYPE_GetOpenFileNameW) Module_GetProcedureAddress(
                        Module_Load(L"comdlg32.dll"),
                        "GetOpenFileNameW"
                        );

                    GetOpenFileNameW( &ofn );

                    imageName = String_FindLastChar(filePath, '\\') + 1;

                    if (SlIniReadBoolean(L"Games", imageName, FALSE, L".\\" PUSH_SETTINGS_FILE))
                        imageName = filePath;

                    // Get free index.

                    games = SlIniReadString(L"Games", NULL, NULL, L".\\" PUSH_SETTINGS_FILE);

                    if (games)
                    {
                        // Get number of games
                        for (i = 0; games[0] != '\0'; i++)
                            games = String_FindLastChar(games, '\0') + 1;
                    }

                    // Increment counter by 1, this is the new index
                    i++;

                    swprintf(indexString, 10, L"%i", i);
                    SlIniWriteString(L"Games", filePath, indexString, L".\\" PUSH_SETTINGS_FILE);
                    GetPathOnly(filePath, path);

                    slash = String_FindLastChar(path, '\\');
                    *slash = '\0';

                    MapFileAndCheckSumW(filePath, &headerSum, &checkSum);

                    Game_Initialize(filePath, &game);
                    Game_SetName(&game, imageName);
                    Game_SetInstallPath(&game, path);
                    Game_SetFlags(&game, GAME_RAMDISK);
                    Game_SetCheckSum(&game, checkSum);

                } break;

            case COMBOBOX_GAMES:
                {
                    if (HIWORD(wParam) == CBN_DROPDOWN)
                    {
                        static BOOLEAN gamesInited = FALSE;

                        if (!gamesInited)
                        {
                            WCHAR *games;
                            INT32   i;

                            games = SlIniReadString(L"Games", 0, 0, L".\\" PUSH_SETTINGS_FILE);

                            if (games)
                            {
                                for (i = 0; games[0] != '\0'; i++)
                                {
                                    PUSH_GAME game;

                                    Game_Initialize(games, &game);

                                    SendMessageW(
                                        MwControlHandles[COMBOBOX_GAMES],
                                        CB_ADDSTRING,
                                        0,
                                        (LONG) game.Name
                                        );

                                    SendMessageW(
                                        MwControlHandles[COMBOBOX_GAMES],
                                        CB_SETITEMDATA,
                                        i,
                                        (LONG) games
                                        );

                                    games = String_FindLastChar(games, '\0') + 1;
                                }
                            }

                            gamesInited = TRUE;
                        }

                    }

                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        OpenCacheWindow();
                    }

                } break;

            case BUTTON_MANUALADD:
                {
                    // The BROWSEINFO struct tells the shell 
                    // how it should display the dialog.
                    BROWSEINFOW bi;
                    HANDLE shell32;

                    Memory_Clear(&bi, sizeof(bi));

                    bi.ulFlags = BIF_USENEWUI;
                    bi.hwndOwner = PushMainWindow->Handle;
                    bi.lpszTitle = L"Select folder for caching";

                    // must call this if using BIF_USENEWUI
                    OleInitialize(NULL);

                    shell32 = Module_Load(L"shell32.dll");

                    SHBrowseForFolderW = (TYPE_SHBrowseForFolderW) Module_GetProcedureAddress(
                        shell32,
                        "SHBrowseForFolderW"
                        );

                    SHGetPathFromIDListW = (TYPE_SHGetPathFromIDListW) Module_GetProcedureAddress(
                        shell32,
                        "SHGetPathFromIDListW"
                        );

                    // Show the dialog and get the itemIDList for the selected folder.
                    ITEMIDLIST *pIDL = SHBrowseForFolderW(&bi);

                    if (pIDL != NULL)
                    {
                        // Create a buffer to store the path, then get the path.
                        WCHAR buffer[260] = { '\0' };
                        
                        if (SHGetPathFromIDListW(pIDL, buffer) != 0)
                        {
                            ManualLoad = (WCHAR*)Memory_Allocate(String_GetSize(buffer));

                            String_Copy(ManualLoad, buffer);
                        }

                        // free the item id list
                        CoTaskMemFree(pIDL);
                    }

                    OleUninitialize();
                    OpenCacheWindow();
                }
                break;
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
