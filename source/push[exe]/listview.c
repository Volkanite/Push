#include <sl.h>
//#include <psdk\windef.h>
//#include <psdk\winuser.h>
//#include <psdk\wingdi.h>
#include <sllistview.h>


#define LVM_DELETECOLUMN        (LVM_FIRST + 28)
#define LVM_GETITEMSTATE        (LVM_FIRST + 44)
#define LVM_GETSUBITEMRECT      (LVM_FIRST + 56)
#define LVM_GETCOLUMNW          (LVM_FIRST + 95)
#define LVM_SETCOLUMNW          (LVM_FIRST + 96)

#define LVHT_ONITEMSTATEICON    0x00000008
#define LVS_EX_GRIDLINES        0x00000001
#define LVIR_BOUNDS             0


HANDLE  ListView_Handle;
UINT8   ListView_Columns;
UINT16  ListView_ItemCount;


VOID ListView_Create(
    DWORD Style,
    int Y,
    int Height,
    HANDLE Parent,
    int Id
    )
{
    RECT rcl;

    // Ensure that the common control DLL is loaded.
    //InitCommonControls();

    // Get the size and position of the parent window
    GetClientRect(Parent, &rcl);

    ListView_Handle = CreateWindowExW(
        0L,
        L"SysListView32",
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | Style,
        0,
        Y,
        rcl.right - rcl.left,
        Height ? Height : rcl.bottom - rcl.top,
        Parent,
        (VOID*)Id,
        0,
        0
        );

    ListView_Columns = 0;
    ListView_ItemCount = 0;
}


VOID ListView_EnableCheckboxes()
{
    SendMessageW(
        ListView_Handle,
        LVM_SETEXTENDEDLISTVIEWSTYLE,
        LVS_EX_CHECKBOXES,
        LVS_EX_CHECKBOXES
        );
}


VOID ListView_EnableGridLines()
{
    SendMessageW(
        ListView_Handle,
        LVM_SETEXTENDEDLISTVIEWSTYLE,
        0,
        LVS_EX_GRIDLINES
        );
}


UINT8 ListView_GetItemText(
    UINT16 Item,
    UINT8 SubItem,
    WCHAR* Text,
    UINT16 TextLength
    )
{
    LVITEMW item;
    unsigned __int8 length;

    item.Mask = LVIF_TEXT;
    item.Item = Item;
    item.SubItem = SubItem;
    item.Text = Text;
    item.TextMax = TextLength;

    SendMessageW(ListView_Handle, LVM_GETITEM, 0, (LONG)&item);

    length = String_GetLength(item.Text);

    return length;
}


VOID ListView_SetItemText(WCHAR* Name, UINT16 Item, UINT8 SubItem)
{
    LVITEMW item;

    item.Mask = LVIF_TEXT;
    item.Item = Item;
    item.SubItem = SubItem;
    item.Text = Name;

    SendMessageW(ListView_Handle, LVM_SETITEM, 0, (LONG)&item);
}


BOOLEAN ListView_GetCheckState( UINT16 Index )
{
    return ((((UINT32)(SendMessageW(
        ListView_Handle,
        LVM_GETITEMSTATE,
        Index,
        LVIS_STATEIMAGEMASK))) >> 12) - 1);
}


INT32 ListView_HitTest( LVHITTESTINFO* HitInfo )
{
    return SendMessageW(ListView_Handle, LVM_HITTEST, 0, (LONG)HitInfo);
}


VOID ListView_AddColumn( WCHAR* Name )
{
    LVCOLUMN column;

    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    column.fmt = LVCFMT_LEFT;
    column.cx = 75;
    column.iSubItem = ListView_Columns;
    column.pszText = Name;

    SendMessageW(
        ListView_Handle,
        LVM_INSERTCOLUMN,
        ListView_Columns,
        (LONG)&column
        );

    ListView_Columns++;
}


UINT16 ListView_AddItem( WCHAR* Name, LONG Param )
{
    LVITEMW item;

    item.Mask = LVIF_TEXT | LVIF_PARAM;
    item.Item = ListView_ItemCount;
    item.SubItem = 0;
    item.Text = Name;
    item.Param = Param;

    SendMessageW(ListView_Handle, LVM_INSERTITEM, 0, (LONG)&item);

    ListView_ItemCount++;

    return item.Item;
}


BOOLEAN ListView_IsCheckBox( POINT Location )
{
    LVHITTESTINFO hitInfo;

    hitInfo.Point = Location;

    ListView_HitTest(&hitInfo);

    if (hitInfo.flags & LVHT_ONITEMSTATEICON)
        return TRUE;
    else
        return FALSE;
}


UINT16 ListView_GetColumnWidth( UINT8 Column )
{
    return SendMessageW(ListView_Handle, LVM_GETCOLUMNWIDTH, Column, 0);
}


VOID ListView_SetColumnWidth( UINT8 Column, INT32 Width )
{
    SendMessageW(ListView_Handle, LVM_SETCOLUMNWIDTH, Column, Width);
}


VOID ListView_SetItemState( UINT16 Item, BOOLEAN State )
{
    LVITEMW item;

    item.StateMask = LVIS_STATEIMAGEMASK;
    item.State = INDEXTOSTATEIMAGEMASK((State)?2:1);

    SendMessageW(ListView_Handle, LVM_SETITEMSTATE, Item, (LONG)&item);
}


VOID ListView_SortItems( VOID* CompareFunction )
{
    SendMessageW(
        ListView_Handle,
        LVM_SORTITEMS,
        1,
        (LONG)CompareFunction
        );
}


VOID ListView_DeleteItem( UINT16 Item )
{
    SendMessageW(ListView_Handle, LVM_DELETEITEM, Item, NULL);

    ListView_ItemCount--;
}


VOID ListView_DeleteAllItems()
{
    SendMessageW(ListView_Handle, LVM_DELETEALLITEMS, NULL, NULL);

    ListView_ItemCount = 0;
}


VOID ListView_DeleteColumn( UINT8 Column )
{
    SendMessageW(ListView_Handle, LVM_DELETECOLUMN, Column, 0);

    ListView_Columns--;
}


VOID ListView_DeleteAllColumns()
{
    UINT8 columnCount;
    int i;

    columnCount = ListView_Columns;

    for (i = 0; i < columnCount; i++)
        ListView_DeleteColumn(0);
}


INT32 ListView_GetColumnId( WCHAR* ColumnName )
{
    int i;

    for (i = 0; i < ListView_Columns; i++)
    {
        WCHAR columnName[260];

        ListView_GetColumnText(i, columnName);

        if (String_Compare(columnName, ColumnName) == 0)
            return i;
    }

    return -1;
}


VOID ListView_SetColumnText(WCHAR* Text)
{
    LVCOLUMN column = { 0 };

    column.mask = LVCF_TEXT;
    column.pszText = Text;

    SendMessageW(ListView_Handle, LVM_SETCOLUMNW, 1, (LONG)&column);
}


VOID ListView_GetColumnText(UINT8 Column, WCHAR* Buffer)
{
    LVCOLUMN column = { 0 };

    column.mask = LVCF_TEXT;
    column.pszText = Buffer;
    column.cchTextMax = 260;

    SendMessageW(ListView_Handle, LVM_GETCOLUMNW, Column, (LONG)&column);
}


UINT32 ListView_GetHeaderWidth( UINT8 Column )
{
    WCHAR columnText[260];
    HANDLE deviceContext;
    SIZE size;

    deviceContext = GetDC(ListView_Handle);

    ListView_GetColumnText(Column, columnText);

    GetTextExtentPoint32W(
        deviceContext,
        columnText,
        String_GetLength(columnText),
        &size
        );

    ReleaseDC(ListView_Handle, deviceContext);

    return size.cx;
}


VOID ListView_GetSubItemRect( UINT16 Item, UINT8 SubItem, RECT* Area )
{
    RECT area;

    area.top = SubItem;
    area.left = LVIR_BOUNDS;

    SendMessageW(ListView_Handle, LVM_GETSUBITEMRECT, Item, (LONG)&area);

    Area->bottom = area.bottom;
    Area->left = area.left;
    Area->right = area.right;
    Area->top = area.top;
}

#define SWP_NOMOVE      0x0002
#define SWP_NOZORDER    0x0004
#define SWP_NOACTIVATE  0x0010
VOID ListView_GrowWindow( HANDLE WindowHandle )
{
    UINT32 columnWidth = 0;
    RECT windowRect, listViewRect;
    UINT8 totalColumns, i;

    totalColumns = ListView_Columns;

    for (i = 0; i < totalColumns; i++)
    {
        columnWidth += ListView_GetColumnWidth(i);
    }

    columnWidth += 50;

    GetWindowRect(WindowHandle, &windowRect);

    if (columnWidth > (UINT32)(windowRect.right - windowRect.left))
    {
        SetWindowPos(
            WindowHandle,
            0,
            0,
            0,
            columnWidth,
            windowRect.bottom - windowRect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
            );

        GetWindowRect(ListView_Handle, &listViewRect);

        SetWindowPos(
            ListView_Handle,
            0,
            0,
            0,
            columnWidth - 15,
            listViewRect.bottom - listViewRect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
            );
    }
}
