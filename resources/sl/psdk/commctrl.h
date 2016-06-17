typedef struct _LVHITTESTINFO {
    POINT Point;
    UINT32  flags;
    int   iItem;
    int   iSubItem;
    int   iGroup;
} LVHITTESTINFO;

typedef struct tagNMLISTVIEW
{
    NMHDR   hdr;
    int     iItem;
    int     iSubItem;
    UINT32    uNewState;
    UINT32    uOldState;
    UINT32    uChanged;
    POINT   ptAction;
    INT32  lParam;
} NMLISTVIEW, *LPNMLISTVIEW;
#define LVM_EDITLABELW          (LVM_FIRST + 118)
#define LVS_EX_GRIDLINES        0x00000001
