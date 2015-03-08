#define IDM_RESTORE 104
#define IDM_EXIT    105


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Minimizes a window to the system tray.
 *
 * \param ParentWindowHandle Optional; Handle to the parent window. 
 * This window will be set invisible.
 * \param IconImage Image for the icon.
 * \param IconText Text to display when mouse hovers over try icon.
 * \param MenuFlags Choose what menu items to display.
 * \param InvisibleWindowHandle Optional; Returns the handle of the 
 * created invisible window.
 * \param TrayIconHandle Optional; Returns the handle of the tray icon.
 */
 
VOID SlTrayMinimize(
    _In_ HANDLE ParentWindowHandle,
    _In_ HANDLE IconImage,
    _In_ WCHAR* IconText,
    _In_ DWORD MenuFlags,
    _Out_ HANDLE* InvisibleWindowHandle,
    _Out_ HANDLE* TrayIconHandle
    );
    
#ifdef __cplusplus
}
#endif
