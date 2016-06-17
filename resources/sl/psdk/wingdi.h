
INTBOOL __stdcall GetTextExtentPoint32W(
  HANDLE hdc,
  WCHAR* lpString,
  int c,
  LPSIZE psizl
  );

#define RGB(r,g,b)          ((DWORD)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
