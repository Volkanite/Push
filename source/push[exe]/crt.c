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

	_iswspace = (TYPE_iswspace)Module_GetProcedureAddress(ntdll, "iswspace");
	ntdll_memcmp = (TYPE_memcmp)Module_GetProcedureAddress(ntdll, "memcmp");
	ntdll_memcpy = (TYPE_memcpy)Module_GetProcedureAddress(ntdll, "memcpy");
	ntdll_memset = (TYPE_memset)Module_GetProcedureAddress(ntdll, "memset");
	ntdll_strcmp = (TYPE_strcmp)Module_GetProcedureAddress(ntdll, "strcmp");
	ntdll_strcpy = (TYPE_strcpy)Module_GetProcedureAddress(ntdll, "strcpy");
	ntdll_strlen = (TYPE_strlen)Module_GetProcedureAddress(ntdll, "strlen");
	ntdll_strncmp = (TYPE_strncmp)Module_GetProcedureAddress(ntdll, "strncmp");
	strncpy = (TYPE_strncpy)Module_GetProcedureAddress(ntdll, "strncpy");
	swscanf_s = (TYPE_swscanf_s)Module_GetProcedureAddress(ntdll, "swscanf_s");
	vswprintf_s = (TYPE_vswprintf_s)Module_GetProcedureAddress(ntdll, "vswprintf_s");
	wcsncat = (TYPE_wcsncat)Module_GetProcedureAddress(ntdll, "wcsncat");
	wcsnlen = (TYPE_wcsnlen)Module_GetProcedureAddress(ntdll, "wcsnlen");
	_wcstol = (TYPE_wcstol)Module_GetProcedureAddress(ntdll, "wcstol");
	__wtoi = (TYPE__wtoi)Module_GetProcedureAddress(ntdll, "_wtoi");
}
