#include <sltypes.h>
#include <slntuapi.h>
#include <slgui.h>
#include <wchar.h>
#include <ini.h>
#include <file.h>

#include "gui.h"
#include "main.h"


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
SlListView* CwListView;


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
            WCHAR *buffer;
            WCHAR gamePath[260];
            UINT32 columnWidth;
            RECT windowRect, treeListRect;

            CwListView = new SlListView(Handle, LISTVIEW);

            CwListView->EnableCheckboxes();

            CwListView->AddColumn(L"File");
            CwListView->AddColumn(L"Size");

            buffer = IniReadSubKey(
                        L"Game Settings", 
                        GetComboBoxData(), 
                        L"Path"
                        );

            wcscpy(gamePath, buffer);
            RtlFreeHeap(PushHeapHandle, 0, buffer);

            if (!FolderExists(gamePath))
            {
                MessageBoxW(0, L"Folder not exist!", 0,0);

                return 0;
            }

            MwBatchFile = new BfBatchFile(GetComboBoxData());

            FsEnumDirectory(gamePath, L"*", BuildGameFilesList);
            ListViewAddItems();

            CwListView->SortItems(ListViewCompareProc);
            CwListView->SetColumnWidth();

            columnWidth = CwListView->GetColumnWidth(0);
            columnWidth += CwListView->GetColumnWidth(1);
            columnWidth += 50;

            GetWindowRect(cacheWindow->Handle, &windowRect);

            if (columnWidth > (UINT32)(windowRect.right - windowRect.left))
            {
                SetWindowPos(cacheWindow->Handle,
                             0, 0, 0,
                             columnWidth,
                             windowRect.bottom - windowRect.top,
                             0x16); // SWP_NOMOVE|
                                    // SWP_NOZORDER|
                                    // SWP_NOACTIVATE

                GetWindowRect(CwListView->Handle, &treeListRect);

                SetWindowPos(
                    CwListView->Handle,
                    0,
                    0,
                    0,
                             columnWidth - 15,
                             treeListRect.bottom - treeListRect.top,
                             0x16);
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

                if (CwListView->IsCheckBox(message->Point))
                    checkbox = TRUE;

                CwListView->GetItemText(message->Item, 0, fileName, 260);
                CwListView->GetItemText(message->Item, 1, fileSize, 260);

                file.Name = fileName;
                file.Bytes = _wtoi(fileSize);

                if (CwListView->GetCheckState(message->Item))
                {
                    MwBatchFile->RemoveItem(&file);

                    if (!checkbox)
                        CwListView->SetItemState(message->Item, FALSE);
                }
                else
                {
                    MwBatchFile->AddItem(&file);

                    if (!checkbox)
                        CwListView->SetItemState(message->Item, TRUE);

                    g_bRecache = TRUE;
                }
            }

        } break;
    }

    return DefWindowProcW(Handle, uMessage, wParam, lParam);
}
