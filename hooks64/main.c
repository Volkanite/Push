#include <sl.h>

#include "push.h"
typedef VOID(*TYPE_InstallOverlayHook)();
TYPE_InstallOverlayHook InstallOverlayHook;

TYPE_iswspace       iswspace;
TYPE_memcmp         memcmp;
TYPE_memcpy         memcpy;
TYPE_memset         memset;
TYPE_strcmp         strcmp;
TYPE_strcpy         strcpy;
TYPE_strlen         strlen;
TYPE_strncmp        strncmp;
TYPE_strncpy        strncpy;
TYPE_swscanf_s      swscanf_s;
TYPE_vswprintf_s    vswprintf_s;
TYPE_wcsncat        wcsncat;
TYPE_wcsnlen        wcsnlen;
TYPE_wcstol         wcstol;
TYPE__wtoi          _wtoi;

TYPE_strcmp         ntdll_strcmp;
TYPE_memcmp         ntdll_memcmp;
TYPE_strncmp        ntdll_strncmp;
TYPE_strlen         ntdll_strlen;


FARPROC __stdcall GetProcAddress(
	HANDLE hModule,
	CHAR*  lpProcName
	);


VOID InitializeCRT()
{
	void* ntdll;

	ntdll = Module_GetHandle(L"ntdll.dll");

	iswspace = (TYPE_iswspace)GetProcAddress(ntdll, "iswspace");
	memcmp = ntdll_memcmp = (TYPE_memcmp)GetProcAddress(ntdll, "memcmp");
	memcpy = (TYPE_memcpy)GetProcAddress(ntdll, "memcpy");
	memset = (TYPE_memset)GetProcAddress(ntdll, "memset");
	strcmp = ntdll_strcmp = (TYPE_strcmp)GetProcAddress(ntdll, "strcmp");
	strcpy = (TYPE_strcpy)GetProcAddress(ntdll, "strcpy");
	strlen = ntdll_strlen = (TYPE_strlen)GetProcAddress(ntdll, "strlen");
	strncmp = ntdll_strncmp = (TYPE_strncmp)GetProcAddress(ntdll, "strncmp");
	strncpy = (TYPE_strncpy)GetProcAddress(ntdll, "strncpy");
	swscanf_s = (TYPE_swscanf_s)GetProcAddress(ntdll, "swscanf_s");
	vswprintf_s = (TYPE_vswprintf_s)GetProcAddress(ntdll, "vswprintf_s");
	wcsncat = (TYPE_wcsncat)GetProcAddress(ntdll, "wcsncat");
	wcsnlen = (TYPE_wcsnlen)GetProcAddress(ntdll, "wcsnlen");
	wcstol = (TYPE_wcstol)GetProcAddress(ntdll, "wcstol");
	_wtoi = (TYPE__wtoi)GetProcAddress(ntdll, "_wtoi");
}

NTSTATUS __stdcall NtDelayExecution(
	BOOLEAN Alertable,
	LARGE_INTEGER* DelayInterval
	);
NTSleep(UINT32 Milliseconds)
{
	LARGE_INTEGER interval;

	interval.QuadPart = Milliseconds * -10000LL;
	NtDelayExecution(FALSE, &interval);
}


int main()
{
	HANDLE overlayLib = NULL;
	void* prcAddress = 0;

	InitializeCRT();

	overlayLib = Module_Load(L"overlay64.dll");
	prcAddress = Module_GetProcedureAddress(overlayLib, "InstallOverlayHook");

	if (prcAddress)
	{
		InstallOverlayHook = (TYPE_InstallOverlayHook)prcAddress;
		InstallOverlayHook();
	}

	NTSleep(0xFFFFFFFF);
}