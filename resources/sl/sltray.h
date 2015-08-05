#define IDM_RESTORE 104
#define IDM_EXIT    105

class Tray
{
public:
	static VOID Minimize( HANDLE ParentWindowHandle,HANDLE IconImage, WCHAR* IconText, DWORD MenuFlags, HANDLE* InvisibleWindowHandle, HANDLE* TrayIconHandle );
};

