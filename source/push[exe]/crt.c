#include <push.h>

FARPROC __stdcall GetProcAddress(HANDLE hModule, CHAR*  lpProcName);

TYPE_iswspace       _iswspace;
TYPE_memcmp         ntdll_memcmp;
TYPE_memcpy         ntdll_memcpy;
TYPE_memset         ntdll_memset;
TYPE_strcmp         ntdll_strcmp;
TYPE_strcpy         ntdll_strcpy;
TYPE_strlen         ntdll_strlen;
TYPE_strncmp        ntdll_strncmp;
TYPE_strncpy        strncpy;
TYPE_swscanf_s      swscanf_s;
TYPE_vswprintf_s    vswprintf_s;
TYPE_wcsncat        wcsncat;
TYPE_wcsnlen        wcsnlen;
TYPE_wcstol         _wcstol;
TYPE__wtoi          __wtoi;


VOID InitializeCRT()
{
	void* ntdll;

	ntdll = Module_GetHandle(L"ntdll.dll");

	_iswspace = (TYPE_iswspace)GetProcAddress(ntdll, "iswspace");
	ntdll_memcmp = (TYPE_memcmp)GetProcAddress(ntdll, "memcmp");
	ntdll_memcpy = (TYPE_memcpy)GetProcAddress(ntdll, "memcpy");
	ntdll_memset = (TYPE_memset)GetProcAddress(ntdll, "memset");
	ntdll_strcmp = (TYPE_strcmp)GetProcAddress(ntdll, "strcmp");
	ntdll_strcpy = (TYPE_strcpy)GetProcAddress(ntdll, "strcpy");
	ntdll_strlen = (TYPE_strlen)GetProcAddress(ntdll, "strlen");
	ntdll_strncmp = (TYPE_strncmp)GetProcAddress(ntdll, "strncmp");
	strncpy = (TYPE_strncpy)GetProcAddress(ntdll, "strncpy");
	swscanf_s = (TYPE_swscanf_s)GetProcAddress(ntdll, "swscanf_s");
	vswprintf_s = (TYPE_vswprintf_s)GetProcAddress(ntdll, "vswprintf_s");
	wcsncat = (TYPE_wcsncat)GetProcAddress(ntdll, "wcsncat");
	wcsnlen = (TYPE_wcsnlen)GetProcAddress(ntdll, "wcsnlen");
	_wcstol = (TYPE_wcstol)GetProcAddress(ntdll, "wcstol");
	__wtoi = (TYPE__wtoi)GetProcAddress(ntdll, "_wtoi");
}
