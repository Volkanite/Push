#include <sl.h>

#include "push.h"
typedef VOID(*TYPE_InstallOverlayHook)();
TYPE_InstallOverlayHook InstallOverlayHook;


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