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
    VOID* Handle, 
    UINT32 uMessage, 
    DWORD wParam, 
    LONG lParam 
    )
{
    switch(uMessage)
    {
    case WM_CREATE:
        {
            UINT32 columnWidth;
            RECT windowRect, treeListRect;
            PUSH_GAME game;

            ListView_Create(0, LISTVIEW, 0, 0, 0);
            ListView_EnableCheckboxes();

            ListView_AddColumn(L"File");
            ListView_AddColumn(L"Size");

            Game_Initialize(GetComboBoxData(), &game);

            if (!FolderExists(game.InstallPath))
            {
                MessageBoxW(0, L"Folder not exist!", 0,0);

                return 0;
            }

            //MwBatchFile = new BfBatchFile(&game);

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

            GetWindowRect(Handle, &windowRect);

            if (columnWidth > (UINT32)(windowRect.right - windowRect.left))
            {
                SetWindowPos(
                    Handle,
                    0,
                    0,
                    0,
                    columnWidth,
                    windowRect.bottom - windowRect.top,
                    SWP_NOMOVE | SWP_NOZORDER| SWP_NOACTIVATE
                    );

                GetWindowRect(ListView_Handle, &treeListRect);

                SetWindowPos(
                    ListView_Handle,
                    0,
                    0,
                    0,
                    columnWidth - 15,
                    treeListRect.bottom - treeListRect.top,
                    SWP_NOMOVE | SWP_NOZORDER| SWP_NOACTIVATE
                    );
            }

        } break;

    case WM_CLOSE:
        {
            //if (MwBatchFile)
                BatchFile_SaveBatchFile();

            break;
        }

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

    return DefWindowProcW(Handle, uMessage, wParam, lParam);
}
