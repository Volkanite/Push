#include <sl.h>

#define COLOR_WINDOW            5


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
    )
{
    VOID *handle;
    WNDCLASSEX windowClass = { 0 };

    windowClass.WndProc = wndProc;
    windowClass.ClassName = className;
    windowClass.Size = sizeof(WNDCLASSEX);
    windowClass.Icon = Icon;
    windowClass.IconSm = NULL;
    windowClass.Background = (HANDLE)(COLOR_WINDOW + 1);

    RegisterClassExW(&windowClass);

    handle = CreateWindowExW(
        ExStyle,
        className,
        windowName,
        /*WS_SYSMENU*/Style,
        ((int)0x80000000), //CW_USEDEFAULT
        NULL,
        width,
        height,
        parent,
        0,
        0,
        0
        );

    if (!handle)
        return NULL;

    ShowWindow(handle, 1);

    return handle;
}


/**
* Subclasses a control.
*
* \param ControlHandle A handle to the control.
* \param NewWndProc A pointer to the function that will be
* the control's new window procedure.
*/

VOID
SlSubClassControl(
    VOID* ControlHandle,
    VOID* NewWndProc
    )
{
    SetPropW(
        ControlHandle,
        L"Wprc",
        (VOID*) GetWindowLongW(ControlHandle, GWL_WNDPROC)
        );

    SetWindowLongW(
        ControlHandle,
        GWL_WNDPROC,
        (LONG) NewWndProc
        );
}


VOID
SlHandleMessages()
{
    MSG message;

    while(GetMessageW(&message, 0,0,0))
    {
        TranslateMessage(&message);

        DispatchMessageW(&message);
    }
}


