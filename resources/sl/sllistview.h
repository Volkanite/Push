#ifndef _LISTVIEW_H
#define _LISTVIEW_H

VOID ListView_Create(DWORD Style, int Y, int Height, HANDLE Parent, int Id);
VOID ListView_EnableCheckboxes();
VOID ListView_EnableGridLines();
UINT8 ListView_GetItemText(UINT16 Item, UINT8 SubItem, WCHAR* Text, UINT16 TextLength);
VOID ListView_SetItemText(WCHAR* Name, UINT16 Item, UINT8 SubItem);
BOOLEAN ListView_GetCheckState(UINT16 Index);
INT32 ListView_HitTest(LVHITTESTINFO* HitInfo);
VOID ListView_AddColumn(WCHAR* Name);
UINT16 ListView_AddItem(WCHAR* Name, LONG Param);
VOID ListView_DeleteItem(UINT16 Item);
VOID ListView_DeleteAllItems();
VOID ListView_DeleteColumn(UINT8 Column);
VOID ListView_DeleteAllColumns();
BOOLEAN ListView_IsCheckBox(POINT Location);
UINT16 ListView_GetColumnWidth(UINT8 Column);
VOID ListView_SetColumnWidth(UINT8 Column, INT32 Width);
VOID ListView_SetItemState(UINT16 Item, BOOLEAN State);
VOID ListView_SortItems(VOID* CompareFunction);
VOID ListView_GetSubItemRect(UINT16 Item, UINT8 SubItem, RECT* Area);
INT32 ListView_GetColumnId(WCHAR* ColumnName);
VOID ListView_GetColumnText(UINT8 Column, WCHAR* Buffer);
VOID ListView_SetColumnText(WCHAR* Text);
UINT32 ListView_GetHeaderWidth(UINT8 Column);
VOID ListView_GrowWindow(HANDLE WindowHandle);

extern HANDLE ListView_Handle;
extern UINT8 ListView_Columns;
extern UINT16 ListView_ItemCount;

#endif

