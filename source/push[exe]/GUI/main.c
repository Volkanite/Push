#include <sl.h>
#include <sltray.h>
#include <file.h>
#include <ramdisk.h>
#include <sldirectory.h>

#include "gui.h"


WINDOW tab;
WINDOW* PushMainWindow;
WINDOW* cacheWindow;
WINDOW fake;


VOID* MwControlHandles[50];
FILE_LIST MwFileList;

VOID Overclock();
/*CONTROL MainControls[] = {
    { L"Button", NULL, BUTTON_ADDGAME, L"Add Game", NULL, 180 },
    { L"Button", NULL, BUTTON_STOPRAMDISK, L"Stop RAMDisk", NULL, 180 },
    { L"ComboBox", CBS_DROPDOWN, COMBOBOX_GAMES, NULL, NULL, 180 },
    { L"Button", NULL, BUTTON_MANUALADD, L"Manual Add", 80, 100 },
    { L"Button", NULL, BUTTON_TRAY, L"To tray..", NULL, 180 },
};*/


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


VOID ListViewAddItems()
{
    FILE_LIST_ENTRY *file = MwFileList;
    WCHAR text[260];
    UINT16 item;

    while (file != NULL)
    {
        item = ListView_AddItem(file->Name, file->Bytes);

        String_Format(text, 260, L"%u", file->Bytes);
        ListView_SetItemText(text, item, 1);

        if (file->Cache)
            ListView_SetItemState(item, TRUE);

        file = file->NextEntry;
    }
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

    String_CopyN(fileName, Information->FileName, fileNameLength);

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

        filePath = (WCHAR*)Memory_Allocate(
            (String_GetLength(Directory) * sizeof(WCHAR)) + sizeof(slash) + Information->FileNameLength
            );

        AppendFileNameToPath(fileName, Directory, filePath);

        fileEntry.Bytes = Information->EndOfFile.u.LowPart;
        fileEntry.Name = filePath;
        fileEntry.Cache = BatchFile_IsBatchedFile(&fileEntry);

        PushAddToFileList(&MwFileList, &fileEntry);
        Memory_Free(filePath);
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


VOID MwCreateMainWindow()
{
    WINDOW pageWindow;
    INT32 i = 0, pageTopOffset = 24;

    //initialize icon handle
    Gui_IconImageHandle = LoadIconW(PushInstance, L"PUSH_ICON");

    // Create Window
    PushMainWindow = (WINDOW*)Memory_Allocate(
                                /*PushHeapHandle,
                                0,*/
                                sizeof(WINDOW)
                                );

    PushMainWindow->lastPos = 0;
    PushMainWindow->Handle = SlCreateWindow(
        0, 
        L"RTSSPush", 
        L"Push", 
        WS_SYSMENU, 
        200, 
        160, 
        MainWndProc,
        0,
        Gui_IconImageHandle
        );

    pageWindow.Handle = PushMainWindow->Handle;
    pageWindow.lastPos = 0;

    Tray_Minimize(
        PushMainWindow->Handle,
        Gui_IconImageHandle,
        L"Push",
        IDM_RESTORE,
        &Gui_InvisibleWindowHandle,
        &Gui_TrayIconHandle
        );
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
    LPBROWSEINFOW lpbi
    );

typedef INTBOOL (__stdcall *TYPE_SHGetPathFromIDListW)(
    PCIDLIST_ABSOLUTE pidl,
    WCHAR*            pszPath
    );


TYPE_SHBrowseForFolderW     SHBrowseForFolderW;
TYPE_SHGetPathFromIDListW   SHGetPathFromIDListW;


    LONG __stdcall OleInitialize(
        VOID* pvReserved
        );

    void __stdcall OleUninitialize(void);

    void __stdcall CoTaskMemFree(
        VOID* pv
        );

    INT32 __stdcall DialogBoxParamW(
        HANDLE hInstance,
        WCHAR*   lpTemplateName,
        HANDLE      hWndParent,
        VOID*   lpDialogFunc,
        LONG    dwInitParam
        );

    void __stdcall ILFree(
        VOID* pidl
        );

WCHAR* ManualLoad;


VOID OpenCacheWindow()
{
    cacheWindow = (WINDOW *)Memory_Allocate(sizeof(WINDOW));

    cacheWindow->lastPos = 0;

    cacheWindow->Handle = SlCreateWindow(
        0,
        L"Cache Manager",
        L"Cache Settings",
        WS_SYSMENU,
        600,
        500,
        CacheWndProc,
        PushMainWindow->Handle,
        NULL
        );
}


typedef INT32 (__stdcall *TYPE_GetOpenFileNameW)( VOID* );
TYPE_GetOpenFileNameW GetOpenFileNameW;
VOID Overclock();


INT32 __stdcall MainWndProc( VOID *hWnd,UINT32 uMessage, UINT32 wParam, LONG lParam )
{
    ITEMIDLIST *pIDL;

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

            case MENU_OVERCLOCK:
                Overclock();
                break;
            case MENU_CACHE:
                OpenCacheWindow();
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

                    if (SlIniReadBoolean(L"Games", imageName, FALSE))
                        imageName = filePath;

                    // Get free index.

                    games = Memory_Allocate(512 * sizeof(WCHAR));

                    Ini_GetString(L"Games", NULL, NULL, games, 512);

                    if (games)
                    {
                        // Get number of games
                        for (i = 0; games[0] != '\0'; i++)
                            games = String_FindLastChar(games, '\0') + 1;
                    }

                    // Increment counter by 1, this is the new index
                    i++;

                    String_Format(indexString, 10, L"%i", i);
                    SlIniWriteString(L"Games", filePath, indexString);
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
                    pIDL = SHBrowseForFolderW(&bi);

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
