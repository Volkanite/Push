#include <sltypes.h>
#include <slnt.h>
#include <sldetours.h>
#include <slntuapi.h>
#include <slmodule.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include <sld3d9.h>

	typedef struct _RGNDATAHEADER {
  DWORD dwSize;
  DWORD iType;
  DWORD nCount;
  DWORD nRgnSize;
  RECT  rcBound;
} RGNDATAHEADER, *PRGNDATAHEADER;
	typedef struct _RGNDATA {
  RGNDATAHEADER rdh;
  char          Buffer[1];
} RGNDATA, *PRGNDATA;
	typedef struct tagPALETTEENTRY {
    BYTE        peRed;
    BYTE        peGreen;
    BYTE        peBlue;
    BYTE        peFlags;
} PALETTEENTRY;
typedef struct _HOOK_PARAMS
{

	D3DPRESENT_PARAMETERS*	presentationParameters;
	VOID*	device;

} HOOK_PARAMS;
typedef LONG HRESULT;

typedef IDirect3D9* (__stdcall *TYPE_Direct3DCreate9)(
	UINT32 SDKVersion);

typedef LONG (__stdcall* TYPE_IDirect3D9_GetAdapterDisplayMode)(
	IDirect3D9* Object,
	UINT32 Adapter,
	D3DDISPLAYMODE* pMode
	);

typedef LONG (__stdcall* TYPE_IDirect3D9_CreateDevice)(
	IDirect3D9 *d3dObj,
	UINT32 Adapter,
	UINT32 DeviceType,
	VOID* hFocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS *pPresentationParameters,
	IDirect3DDevice9 **ppReturnedDeviceInterface);

typedef LONG (__stdcall *TYPE_IDirect3DDevice9_Present) (
	IDirect3DDevice9* pDevice,
	const RECT* pSourceRect,
	const RECT* pDestRect,
	VOID* hDestWindowOverride,
	const RGNDATA* pDirtyRegion
);

typedef LONG (__stdcall *TYPE_IDirect3DDevice9_Reset) (
	IDirect3DDevice9* pDevice,
	D3DPRESENT_PARAMETERS* pPresentationParameters
);

typedef LONG (__stdcall *TYPE_IDirect3DDevice9_GetSwapChain)(
	IDirect3DDevice9* device,
	UINT32 iSwapChain,
	IDirect3DSwapChain9** pSwapChain
	);


//IDirect3DSwapChain9

typedef LONG (__stdcall *TYPE_IDirect3DSwapChain9_Present)(
	IDirect3DSwapChain9* OBJECT_IDirect3DSwapChain9,
	const RECT* pSourceRect,
	const RECT* pDestRect,
	VOID* hDestWindowOverride,
	const RGNDATA* pDirtyRegion,
	unsigned long dwFlags
	);

typedef LONG (__stdcall *TYPE_IDirect3DSwapChain9_GetDevice)(
	IDirect3DSwapChain9* OBJECT_IDirect3DSwapChain9,
	IDirect3DDevice9** ppDevice
	);


/*Callbacks*/

typedef VOID (*TYPE_PresentCallback) (
	IDirect3DDevice9* OBJECT_IDirect3DDevice9
	);

typedef VOID (*TYPE_ResetCallback)(
	D3DPRESENT_PARAMETERS* PresentationParameters
	);


TYPE_Direct3DCreate9                    HkDirect3DCreate9;
TYPE_IDirect3D9_GetAdapterDisplayMode	IDirect3D9_GetAdapterDisplayMode;
TYPE_IDirect3D9_CreateDevice			HkIDirect3D9_CreateDevice;
TYPE_IDirect3DDevice9_GetSwapChain		IDirect3DDevice9_GetSwapChain;
TYPE_IDirect3DSwapChain9_Present		HkIDirect3DSwapChain9_Present;
TYPE_IDirect3DSwapChain9_GetDevice		IDirect3DSwapChain9_GetDevice;
TYPE_IDirect3DDevice9_Present				HkIDirect3DDevice9_Present = 0;
TYPE_IDirect3DDevice9_Reset					HkIDirect3DDevice9_Reset = 0;

TYPE_PresentCallback	HkIDirect3DDevice9_PresentCallback;
TYPE_PresentCallback	HkIDirect3DSwapChain9_PresentCallback;
TYPE_ResetCallback		HkIDirect3DDevice9_ResetCallback;
TYPE_ResetCallback		HkIDirect3D9_CreateDeviceCallback;

HOOK_PARAMS hookParams;
D3DPRESENT_PARAMETERS PresentParams;

#define SUCCEEDED(hr) (((LONG)(hr)) >= 0)
extern "C" VOID* __stdcall GetDesktopWindow(void);

LONG
__stdcall
IDirect3DDevice9_PresentHook(
	IDirect3DDevice9* OBJECT_IDirect3DDevice9,
	const RECT* pSourceRect,
	const RECT* pDestRect,
	VOID *hDestWindowOverride,
	const RGNDATA* pDirtyRegion
)
{
	HkIDirect3DDevice9_PresentCallback(
		OBJECT_IDirect3DDevice9
		);

	return HkIDirect3DDevice9_Present(
			OBJECT_IDirect3DDevice9,
			pSourceRect,
			pDestRect,
			hDestWindowOverride,
			pDirtyRegion
			);
}


LONG
__stdcall
IDirect3DDevice9_ResetHook(
	IDirect3DDevice9* pDevice,
	D3DPRESENT_PARAMETERS* PresentationParameters
)
{
	HkIDirect3DDevice9_ResetCallback( PresentationParameters );

	return HkIDirect3DDevice9_Reset(
			pDevice,
			PresentationParameters
			);
}


LONG
__stdcall
IDirect3DSwapChain9_PresentHook(
	IDirect3DSwapChain9 *OBJECT_IDirect3DSwapChain9,
	const RECT *pSourceRect,
	const RECT *pDestRect,
	VOID* hDestWindowOverride,
	const RGNDATA *pDirtyRegion,
	unsigned long dwFlags
	)
{
	static IDirect3DDevice9 *OBJECT_IDirect3DDevice9 = NULL;

	if (!OBJECT_IDirect3DDevice9)
	{
		VOID **vmt;

		vmt = (VOID**) OBJECT_IDirect3DSwapChain9;
		vmt = (VOID**) vmt[0];

		IDirect3DSwapChain9_GetDevice = (TYPE_IDirect3DSwapChain9_GetDevice) vmt[8];

		IDirect3DSwapChain9_GetDevice(
			OBJECT_IDirect3DSwapChain9,
			&OBJECT_IDirect3DDevice9
			);
	}

	HkIDirect3DSwapChain9_PresentCallback( OBJECT_IDirect3DDevice9 );

	return HkIDirect3DSwapChain9_Present(
			OBJECT_IDirect3DSwapChain9,
			pSourceRect,
			pDestRect,
			hDestWindowOverride,
			pDirtyRegion,
			dwFlags
			);
}


LONG __stdcall IDirect3D9_CreateDeviceHook(
	IDirect3D9* D3D9,
	UINT32 Adapter,
	UINT32 DeviceType,
	VOID* FocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS *PresentationParameters,
	IDirect3DDevice9 **ReturnedDeviceInterface
	)
{
    HRESULT hr;

	HkIDirect3D9_CreateDeviceCallback( PresentationParameters );

    hr = HkIDirect3D9_CreateDevice(
            D3D9,
            Adapter,
            DeviceType,
            FocusWindow,
            BehaviorFlags,
            PresentationParameters,
            ReturnedDeviceInterface
            );

	return hr;
}


IDirect3D9* __stdcall Direct3DCreate9Hook( UINT32 SDKVersion )
{
    IDirect3D9* d3d9 = NULL;
    VOID **vmt;
    SlHookManager hookManager;

    d3d9 = HkDirect3DCreate9(SDKVersion);

    if (d3d9 == NULL)
        return NULL;

    // hook
	vmt = (VOID**) d3d9;
	vmt = (VOID**) vmt[0];

	if (HkIDirect3D9_CreateDeviceCallback)
	{
        HkIDirect3D9_CreateDevice = (TYPE_IDirect3D9_CreateDevice) hookManager.DetourFunction(
                                        (BYTE*)vmt[16],
                                        (BYTE*)IDirect3D9_CreateDeviceHook
                                        );
	}

    return d3d9;
}


VOID
SlHookD3D9(
	VOID* IDirect3DDevice9_PresentCallback,
	VOID* IDirect3DSwapChain9_PresentCallback,
	VOID* IDirect3DDevice9_ResetCallback,
	VOID* IDirect3D9_CreateDeviceCallback
	)
{
	VOID *base = NULL, **vmt;
	IDirect3D9 *d3d = NULL;
	D3DDISPLAYMODE d3dDisplayMode;
	D3DPRESENT_PARAMETERS d3dPresentParameters = {0};
	LONG result;
	IDirect3DDevice9 *OBJECT_IDirect3DDevice9;
	SlHookManager hookManager;

	HkIDirect3DDevice9_PresentCallback = (TYPE_PresentCallback) IDirect3DDevice9_PresentCallback;
	HkIDirect3DSwapChain9_PresentCallback = (TYPE_PresentCallback) IDirect3DSwapChain9_PresentCallback;
	HkIDirect3DDevice9_ResetCallback = (TYPE_ResetCallback) IDirect3DDevice9_ResetCallback;
	HkIDirect3D9_CreateDeviceCallback = (TYPE_ResetCallback) IDirect3D9_CreateDeviceCallback;

	// Get module handle
	base = SlLoadLibrary(L"d3d9.dll");

	// Get IDirect3D9
	HkDirect3DCreate9 = (TYPE_Direct3DCreate9) GetProcAddress(base, "Direct3DCreate9");
	d3d = HkDirect3DCreate9(32);

	// Get IDirect3DDevice9
	vmt = (VOID**) d3d;
	vmt = (VOID**) vmt[0];

	IDirect3D9_GetAdapterDisplayMode = (TYPE_IDirect3D9_GetAdapterDisplayMode) vmt[8];
	HkIDirect3D9_CreateDevice = (TYPE_IDirect3D9_CreateDevice) vmt[16];

	IDirect3D9_GetAdapterDisplayMode(
		d3d,
		D3DADAPTER_DEFAULT,
		&d3dDisplayMode
		);

	d3dPresentParameters.Windowed = TRUE;
	d3dPresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPresentParameters.BackBufferFormat = d3dDisplayMode.Format;

	result = HkIDirect3D9_CreateDevice(
		d3d,
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		GetDesktopWindow(),
		D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
		&d3dPresentParameters,
		&OBJECT_IDirect3DDevice9
		);

	if (!SUCCEEDED(result))
	    return;

	if (IDirect3D9_CreateDeviceCallback)
	{
        HkIDirect3D9_CreateDevice = (TYPE_IDirect3D9_CreateDevice) hookManager.DetourFunction(
                                        (BYTE*)vmt[16],
                                        (BYTE*)IDirect3D9_CreateDeviceHook
                                        );
	}

	// Get method addresses
	vmt = (VOID**) OBJECT_IDirect3DDevice9;
	vmt = (VOID**) vmt[0];

	if (IDirect3DDevice9_PresentCallback)
	{
        HkIDirect3DDevice9_Present = (TYPE_IDirect3DDevice9_Present) hookManager.DetourFunction(
                                        (BYTE*)vmt[17],
                                        (BYTE*)IDirect3DDevice9_PresentHook
                                        );
	}

	if (IDirect3DDevice9_ResetCallback)
	{
        HkIDirect3DDevice9_Reset = (TYPE_IDirect3DDevice9_Reset) hookManager.DetourFunction(
                                    (BYTE*)vmt[16],
                                    (BYTE*)IDirect3DDevice9_ResetHook
                                    );
	}

	if (IDirect3DSwapChain9_PresentCallback)
	{
		IDirect3DSwapChain9 *OBJECT_IDirect3DSwapChain9;

		IDirect3DDevice9_GetSwapChain = (TYPE_IDirect3DDevice9_GetSwapChain) vmt[14];

		IDirect3DDevice9_GetSwapChain(OBJECT_IDirect3DDevice9, 0, &OBJECT_IDirect3DSwapChain9);

		vmt = (VOID**) OBJECT_IDirect3DSwapChain9;
		vmt = (VOID**) vmt[0];

        HkIDirect3DSwapChain9_Present = (TYPE_IDirect3DSwapChain9_Present) hookManager.DetourFunction(
                                            (BYTE*)vmt[3],
                                            (BYTE*)IDirect3DSwapChain9_PresentHook
                                            );
	}
}
