#ifndef _LISTVIEW_H
#define _LISTVIEW_H

class ListView
{
public:
    static VOID Create(VOID* Parent, UINT8 Id, DWORD Style, UINT16 Height);
    static VOID EnableCheckboxes();
    static VOID EnableGridLines();
    static VOID GetItemText(UINT16 Item, UINT8 SubItem, WCHAR* Text, UINT16 TextLength);
    static BOOLEAN GetCheckState(UINT16 Index);
    static INT32 HitTest(LVHITTESTINFO* HitInfo);
    static VOID AddColumn(WCHAR* Name);
    static UINT16 AddItem(WCHAR* Name, LONG Param);
    static VOID DeleteItem( UINT16 Item );
    static VOID DeleteAllItems();
    static VOID DeleteColumn( UINT8 Column );
    static BOOLEAN IsCheckBox(POINT Location);
    static UINT16 GetColumnWidth(UINT8 Column);
    static VOID SetColumnWidth(UINT8 Column, INT32 Width);
    static VOID SetItemText(WCHAR* Name, UINT16 Item, UINT8 SubItem);
    static VOID SetItemState(UINT16 Item, BOOLEAN State);
    static VOID SortItems(VOID* CompareFunction);
    static VOID GetSubItemRect( UINT16 Item, UINT8 SubItem, RECT* Area );
    static VOID SetColumnText(WCHAR* Text);
    static VOID GetColumnText(UINT8 Column, WCHAR* Buffer);
    static UINT32 GetHeaderWidth( UINT8 Column );
    static VOID GrowWindow( HANDLE WindowHandle );

    static HANDLE Handle;
    static UINT8 Columns;
    static UINT16 ItemCount;
};

#endif

