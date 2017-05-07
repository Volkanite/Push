#include <sl.h>
#include <wchar.h>
#include <file.h>
#include <sldirectory.h>

#include "gui.h"
#include "main.h"


#define LISTVIEW 240

extern WINDOW* cacheWindow;


#define SWP_NOMOVE      0x0002
#define SWP_NOZORDER    0x0004
#define SWP_NOACTIVATE  0x0010


LONG __stdcall CacheWndProc( 
    VOID* hWnd, 
    UINT32 uMessage, 
    DWORD wParam, 
    LONG lParam 
    )
{
    static HANDLE comboBoxHandle;

    switch(uMessage)
    {
    case WM_CREATE:
        {
            ListView_Create(0, 0, 300, hWnd, LISTVIEW);
            ListView_EnableCheckboxes();

            ListView_AddColumn(L"File");
            ListView_AddColumn(L"Size");

            comboBoxHandle = CreateWindowExW(
                0,
                L"ComboBox",
                NULL,
                CBS_DROPDOWN | WS_CHILD | WS_VISIBLE,
                20, 400, 200, 20,
                hWnd,
                (VOID*) COMBOBOX_GAMES,
                NULL,
                NULL
                );

        } break;

    case WM_CLOSE:
        {
            //if (MwBatchFile)
                BatchFile_SaveBatchFile();

            break;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case COMBOBOX_GAMES:
        {
            if (HIWORD(wParam) == CBN_DROPDOWN)
            {
                static BOOLEAN gamesInited = FALSE;

                if (!gamesInited)
                {
                    GAME_LIST gameList;
                    PUSH_GAME* game;
                    UINT8 i;

                    i = 0;

                    gameList = Game_GetGames();

                    while (gameList != NULL)
                    {
                        game = gameList->Game;

                        SendMessageW(
                            comboBoxHandle, 
                            CB_ADDSTRING, 
                            0,
                            (LONG)game->Name
                            );

                        SendMessageW(
                            comboBoxHandle,
                            CB_SETITEMDATA,
                            i,
                            (LONG)game->ExecutablePath
                            );

                        gameList = gameList->NextEntry;
                        i++;
                    }

                    gamesInited = TRUE;
                }

            }

            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                PUSH_GAME game;
                UINT32 columnWidth;
                RECT windowRect, treeListRect;
                INT32 iIndex;
                wchar_t* comboBoxText;

                iIndex = SendMessageW(
                    comboBoxHandle, 
                    CB_GETCURSEL, 
                    0, 0
                    );

                comboBoxText = (WCHAR*)SendMessageW(
                    comboBoxHandle,
                    CB_GETITEMDATA,
                    iIndex,
                    0
                    );

                Game_Initialize(comboBoxText, &game);

                if (!FolderExists(game.InstallPath))
                {
                    MessageBoxW(0, L"Folder not exist!", 0, 0);

                    return 0;
                }

                Directory_Enum(
                    ManualLoad ? ManualLoad : game.InstallPath,
                    L"*",
                    BuildGameFilesList
                    );

                ListViewAddItems();

                ListView_SortItems(ListViewCompareProc);
                ListView_SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

                columnWidth = ListView_GetColumnWidth(0);
                columnWidth += ListView_GetColumnWidth(1);
                columnWidth += 50;

                GetWindowRect(hWnd, &windowRect);

                if (columnWidth > (UINT32)(windowRect.right - windowRect.left))
                {
                    SetWindowPos(
                        hWnd,
                        0,
                        0,
                        0,
                        columnWidth,
                        windowRect.bottom - windowRect.top,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
                        );

                    GetWindowRect(ListView_Handle, &treeListRect);

                    SetWindowPos(
                        ListView_Handle,
                        0,
                        0,
                        0,
                        columnWidth - 15,
                        treeListRect.bottom - treeListRect.top,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
                        );
                }

                BatchFile_Initialize(&game);
            }

        } break;
        }
        break;

    case WM_NOTIFY:
        {
            NMLISTVIEW *message = (NMLISTVIEW*)lParam;

            if (wParam == LISTVIEW && message->hdr.code == NM_CLICK)
            {
                WCHAR fileName[260];
                WCHAR fileSize[260];
                BOOLEAN checkbox = FALSE;
                FILE_LIST_ENTRY file;

                if (ListView_IsCheckBox(message->ptAction))
                    checkbox = TRUE;

                ListView_GetItemText(message->iItem, 0, fileName, 260);
                ListView_GetItemText(message->iItem, 1, fileSize, 260);

                file.Name = fileName;
                file.Bytes = wcstol(fileSize, NULL, 10);

                if (ListView_GetCheckState(message->iItem))
                {
                    BatchFile_RemoveItem(&file);

                    if (!checkbox)
                        ListView_SetItemState(message->iItem, FALSE);
                }
                else
                {
                    BatchFile_AddItem(&file);

                    if (!checkbox)
                        ListView_SetItemState(message->iItem, TRUE);

                    g_bRecache = TRUE;
                }
            }

        } break;
    }

    return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}
