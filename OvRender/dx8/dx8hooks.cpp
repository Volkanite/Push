#include <Windows.h>
#include <d3d8.h>
#include <sldetours.h>
#include <slmodule.h>

#include "dx8hooks.h"


typedef IDirect3D8* (WINAPI* TYPE_Direct3DCreate8)(
	UINT SDKVersion
	);

typedef HRESULT (WINAPI *TYPE_IDirect3DDevice8_Present) (
    IDirect3DDevice8* pDevice,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion
	);

typedef HRESULT (WINAPI *TYPE_IDirect3DDevice8_Reset) (
    IDirect3DDevice8 *pDevice,
    D3DPRESENT_PARAMETERS* pPresentationParameters
	);





TYPE_Direct3DCreate8			HkDirect3DCreate8;
TYPE_IDirect3DDevice8_Present	HkIDirect3DDevice8_Present;
TYPE_IDirect3DDevice8_Reset		HkIDirect3DDevice8_Reset;

HK_IDIRECT3DDEVICE8_PRESENT_CALLBACK	HkIDirect3DDevice8_PresentCallback;
HK_IDIRECT3DDEVICE8_RESET_CALLBACK		HkIDirect3DDevice8_ResetCallback;


LONG WINAPI IDirect3DDevice8_PresentHook(
    IDirect3DDevice8 *Device,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion
    )
{
	HkIDirect3DDevice8_PresentCallback( Device );

    return HkIDirect3DDevice8_Present( 
			Device,
			pSourceRect,
			pDestRect,
			hDestWindowOverride,
			pDirtyRegion 
			);
}


HRESULT WINAPI IDirect3DDevice8_ResetHook(
    IDirect3DDevice8 *pDevice,
    D3DPRESENT_PARAMETERS *pPresentationParameters
)
{
	HkIDirect3DDevice8_ResetCallback();

    return HkIDirect3DDevice8_Reset(pDevice, pPresentationParameters);
}


VOID HookD3D8( 
	HK_IDIRECT3DDEVICE8_PRESENT_CALLBACK IDirect3DDevice8_PresentCallback, 
	HK_IDIRECT3DDEVICE8_RESET_CALLBACK IDirect3DDevice8_ResetCallback
	)
{
	VOID **vmt;
	IDirect3D8 *d3d8 = NULL;
	D3DDISPLAYMODE d3dDisplayMode;
	D3DPRESENT_PARAMETERS d3dPresentParameters = {0};
	LONG result;
	IDirect3DDevice8 *OBJECT_IDirect3DDevice8;
	SlHookManager hookManager;
	HMODULE base = NULL;

	HkIDirect3DDevice8_PresentCallback = IDirect3DDevice8_PresentCallback;
	HkIDirect3DDevice8_ResetCallback = IDirect3DDevice8_ResetCallback;

	// Get module handle
	base = (HMODULE) SlLoadLibrary(L"d3d8.dll");

	// Get IDirect3D8

	HkDirect3DCreate8 = (TYPE_Direct3DCreate8) GetProcAddress(base, "Direct3DCreate8");
	d3d8 = HkDirect3DCreate8(D3D_SDK_VERSION);

	// Get IDirect3DDevice8

	d3d8->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3dDisplayMode);

	d3dPresentParameters.Windowed = TRUE;
	d3dPresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPresentParameters.BackBufferFormat = d3dDisplayMode.Format;

	result = d3d8->CreateDevice(
				D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL,
				GetDesktopWindow(),
				D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
				&d3dPresentParameters,
				&OBJECT_IDirect3DDevice8
				);

	if (!SUCCEEDED(result))
	{
		OutputDebugString(L"[OVRENDER] IDirect3D8::CreateDevice failed!");

	    return;
	}

	// Get method addresses

	vmt = (VOID**) OBJECT_IDirect3DDevice8;
	vmt = (VOID**) vmt[0];

    if (IDirect3DDevice8_PresentCallback)
    {
        HkIDirect3DDevice8_Present = (TYPE_IDirect3DDevice8_Present) hookManager.DetourFunction(
                            (BYTE*)vmt[15],
                            (BYTE*)IDirect3DDevice8_PresentHook
                            );
    }

    if (IDirect3DDevice8_ResetCallback)
    {
        HkIDirect3DDevice8_Reset = (TYPE_IDirect3DDevice8_Reset) hookManager.DetourFunction(
                        (BYTE*)vmt[14],
                        (BYTE*)IDirect3DDevice8_ResetHook
                        );
    }
}