#include <sltypes.h>
#include <slntuapi.h>
#include <slgui.h>
#include <wchar.h>

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
            CwListView = new SlListView(Handle, LISTVIEW);

            CwListView->EnableCheckboxes();

            CwListView->AddColumn(L"File");
            CwListView->AddColumn(L"Size");

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
