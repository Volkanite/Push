#define OFN_EXPLORER 0x80000
#define OFN_FILEMUSTEXIST 0x1000
#define OFN_HIDEREADONLY 4
#define OFN_NOCHANGEDIR 8
typedef UINT_B(__stdcall *LPOFNHOOKPROC) (VOID*, UINT32, UINT_B, UINT_B);

typedef struct _OPENFILENAME
{
  DWORD         lStructSize;
  VOID*         hwndOwner;
  VOID*         hInstance;
  WCHAR*        lpstrFilter;
  WCHAR*        lpstrCustomFilter;
  DWORD         nMaxCustFilter;
  DWORD         nFilterIndex;
  WCHAR*        lpstrFile;
  DWORD         nMaxFile;
  WCHAR*        lpstrFileTitle;
  DWORD         nMaxFileTitle;
  WCHAR*        lpstrInitialDir;
  WCHAR*        Title;
  DWORD         Flags;
  WORD          nFileOffset;
  WORD          nFileExtension;
  WCHAR*        lpstrDefExt;
  UINT_B  lCustData;
  LPOFNHOOKPROC lpfnHook;
  WCHAR*        lpTemplateName;
  VOID*         pvReserved;
  DWORD         dwReserved;
  DWORD         FlagsEx;

} OPENFILENAMEW;