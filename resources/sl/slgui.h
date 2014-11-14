#ifdef __cplusplus
extern "C" {
#endif

VOID* SlCreateWindow(
    DWORD ExStyle,
    WCHAR *className,
    WCHAR *windowName,
    INT32 width,
    INT32 height,
    VOID *wndProc,
    VOID *parent,
    VOID* Icon
     );


/**
* Subclasses a control.
*
* \param ControlHandle A handle to the control.
* \param NewWndProc A pointer to the function that will be
* the control's new window procedure.
*/
VOID SlSubClassControl(
    VOID* ControlHandle,
    VOID* NewWndProc
    );

 VOID SlHandleMessages(
     );

     
 #ifdef __cplusplus
 }
 #endif
 

class SlListView{
    UINT8 Columns;
    VOID Initialize( VOID* Parent, UINT8 Id, DWORD Style );

public:
    VOID* Handle;
    UINT16 Items;

    SlListView( VOID* Parent, UINT8 Id );
    SlListView( VOID* Parent, UINT8 Id, DWORD Style );
    VOID AddColumn( WCHAR* Name );
    UINT16 AddItem( WCHAR* Name, LONG Param );
    VOID EnableCheckboxes();
    BOOLEAN GetCheckState( UINT16 Index );
    VOID GetItemState();
    VOID SetItemState( UINT16 Item, BOOLEAN State );
    VOID GetItemText( UINT16 Item, UINT8 SubItem, WCHAR* Text, UINT16 TextLength );
    UINT16 GetColumnWidth( UINT8 Column );
    VOID SetColumnWidth();
    INT32 HitTest( LVHITTESTINFO* HitInfo );
    BOOLEAN IsCheckBox( POINT Location );
    VOID SetItem( WCHAR* Name, UINT16 Item, UINT8 SubItem );
    VOID SortItems( VOID* CompareFunction );
};





