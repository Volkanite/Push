#include "ddrawoverlay.h"
#include <detourxs.h>
#include <ddraw.h>


typedef HRESULT(WINAPI *TYPE_DirectDrawCreate) (
	GUID FAR *, 
	LPDIRECTDRAW FAR *, 
	IUnknown FAR *
	);

TYPE_DirectDrawCreate pDirectDrawCreate;


HRESULT WINAPI DirectDrawCreate_Hook(
	GUID FAR *lpGUID, 
	LPDIRECTDRAW FAR *lplpDD, 
	IUnknown FAR *pUnkOuter
	)
{
	HRESULT returnValue = 0;

	OutputDebugStringW(L"[OVRENDER] DirectDrawCreate Called!");
	returnValue = pDirectDrawCreate(lpGUID, lplpDD, pUnkOuter);

	return returnValue;
}


DDrawOverlay::DDrawOverlay( OV_RENDER RenderFunction )
{
	HMODULE base;
	DetourXS *detour;
	OutputDebugStringW(L"[OVRENDER] Creating DirectDraw Overlay!");
	base = (HMODULE)LoadLibraryW(L"ddraw.dll");
	
	detour = new DetourXS(
		GetProcAddress(base, "DirectDrawCreateEx"), 
		DirectDrawCreate_Hook
		);

	pDirectDrawCreate = (TYPE_DirectDrawCreate)detour->GetTrampoline();

	UserRenderFunction = RenderFunction;
}


VOID
DDrawOverlay::DrawText(WCHAR* Text)
{
	DrawText(Text, FALSE);
}


VOID DDrawOverlay::DrawText(
	WCHAR* Text,
	DWORD Color
	)
{
	DrawText(Text, 20, Line, Color);

	Line += 15;
}


VOID DDrawOverlay::DrawText(
	WCHAR* Text,
	int X,
	int Y,
	DWORD Color
	)
{

}


VOID
DDrawOverlay::Begin()
{

}


VOID
DDrawOverlay::End()
{

}


VOID*
DDrawOverlay::GetDevice()
{
	return NULL;
}