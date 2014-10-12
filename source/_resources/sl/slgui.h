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
* Creates a Window along with a List View Control.
*
* \param WindowName The name of the window.
* \param WndProc The window procedure.
* \param Style The style of the List View Control.
* \li \c LVS_EDITLABELS Create a List View Control with editable labels.
* \param Checkboxes
* \li \c TRUE The List View Control will be created with checkboxes.
* \li \c FALSE The List View Control will be created without checkboxes
* \param ColumnNames A pointer to an array of the List View Control's column names.
* \param ColumnCount Amount of columns to create.
* \param Id An identifier for the List View Control.
* \param WindowHandle A pointer to the handle that will receive the created Window handle.
* \return Handle to List View Control.
*/
/*VOID* SlCreateWindowWithListView(
    WCHAR* WindowName,
    VOID* WndProc,
    DWORD Style,
    BOOLEAN Checkboxes,
    WCHAR** ColumnNames,
    UINT8 ColumnCount,
    UINT8 Id,
    VOID** WindowHandle
    );*/


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
    UINT16 Items;

    VOID Initialize( VOID* Parent, UINT8 Id, DWORD Style );

public:
    VOID* Handle;

    SlListView( VOID* Parent, UINT8 Id );
    SlListView( VOID* Parent, UINT8 Id, DWORD Style );
    VOID AddColumn( WCHAR* Name );
    UINT16 AddItem( WCHAR* Name, LONG Param );
    VOID EnableCheckboxes();
    BOOLEAN GetCheckState( UINT16 Index );
    VOID GetItemCount();
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





