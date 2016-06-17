#ifdef __cplusplus
extern "C" {
#endif

VOID* SlCreateWindow(
    DWORD ExStyle,
    WCHAR *className,
    WCHAR *windowName,
    DWORD Style,
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
#include "sllistview.h"
 







