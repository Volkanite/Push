#include <sl.h>
#include <wchar.h>
#include <file.h>
#include <sldirectory.h>

#include "gui.h"


#define LISTVIEW 240

typedef struct tagNMLISTVIEW {
    NMHDR  hdr;
    int    Item;
    int    iSubItem;
    UINT32   uNewState;
    UINT32   uOldState;
    UINT32   uChanged;
    POINT  Point;
    LONG lParam;
} NMLISTVIEW, *LPNMLISTVIEW;

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

            ListView::Create(Handle, LISTVIEW, 0, 0);
            ListView::EnableCheckboxes();

            ListView::AddColumn(L"File");
            ListView::AddColumn(L"Size");

            Game::Initialize(GetComboBoxData(), &game);

            if (!FolderExists(game.InstallPath))
            {
                MessageBoxW(0, L"Folder not exist!", 0,0);

                return 0;
            }

            MwBatchFile = new BfBatchFile(&game);

            Directory::Enum(game.InstallPath, L"*", BuildGameFilesList);
            ListViewAddItems();

            ListView::SortItems(ListViewCompareProc);
            ListView::SetColumnWidth(0,0);

            columnWidth = ListView::GetColumnWidth(0);
            columnWidth += ListView::GetColumnWidth(1);
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

                GetWindowRect(ListView::Handle, &treeListRect);

                SetWindowPos(
                    ListView::Handle,
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
            MwBatchFile->SaveBatchFile();

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

                if (ListView::IsCheckBox(message->Point))
                    checkbox = TRUE;

                ListView::GetItemText(message->Item, 0, fileName, 260);
                ListView::GetItemText(message->Item, 1, fileSize, 260);

                file.Name = fileName;
                //file.Bytes = _wtoi(fileSize);
                file.Bytes = wcstol(fileSize, NULL, 10);

                if (ListView::GetCheckState(message->Item))
                {
                    MwBatchFile->RemoveItem(&file);

                    if (!checkbox)
                        ListView::SetItemState(message->Item, FALSE);
                }
                else
                {
                    MwBatchFile->AddItem(&file);

                    if (!checkbox)
                        ListView::SetItemState(message->Item, TRUE);

                    g_bRecache = TRUE;
                }
            }

        } break;
    }

    return DefWindowProcW(Handle, uMessage, wParam, lParam);
}
