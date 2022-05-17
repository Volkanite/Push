#include <Windows.h>
#include <DXGI.h>
#include <D3D10.h>
#include <hexus.h>

#include "..\overlay.h"
#include "dxgihook.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d10.lib")


typedef HRESULT (__stdcall* TYPE_IDXGISwapChain_Present0) (
    IDXGISwapChain *SwapChain,
    UINT SyncInterval,
    UINT Flags
    );

typedef HRESULT(__stdcall* TYPE_IDXGISwapChain_Present1) (
	IDXGISwapChain *SwapChain,
	UINT SyncInterval,
	UINT Flags,
	const void* pPresentParameters
	);

typedef HRESULT(__stdcall* TYPE_IDXGISwapChain_ResizeBuffers) (
    IDXGISwapChain *SwapChain,
    UINT BufferCount,
    UINT Width,
    UINT Height,
    DXGI_FORMAT NewFormat,
    UINT SwapChainFlags
    );

TYPE_IDXGISwapChain_Present0         HkIDXGISwapChain_Present;
TYPE_IDXGISwapChain_Present1		_Present1;
TYPE_IDXGISwapChain_ResizeBuffers   HkIDXGISwapChain_ResizeBuffers;

typedef VOID (*HK_IDXGISWAPCHAIN_CALLBACK)(
IDXGISwapChain* SwapChain
);
HK_IDXGISWAPCHAIN_CALLBACK HkIDXGISwapChain_PresentCallback;
HK_IDXGISWAPCHAIN_CALLBACK HkIDXGISwapChain_ResizeBuffersCallback;


VOID OvLog(const wchar_t* Format, ...);
DWORD FindPattern(WCHAR* Module, char pattern[], char mask[]);
DWORD64 FindPattern64(WCHAR* Module, char pattern[], char mask[]);
void ReplaceVirtualMethod(void **VTable, int Function, void *Detour);


HRESULT __stdcall IDXGISwapChain_PresentHook(
    IDXGISwapChain* SwapChain,
    UINT SyncInterval,
    UINT Flags
    )
{
    static BOOLEAN init = FALSE;

    if (!init)
    {
        init = TRUE;

        OvLog(L"Hook called @ IDXGISwapChain_Present");
    }

    if (HkIDXGISwapChain_PresentCallback)
    {
        HkIDXGISwapChain_PresentCallback(SwapChain);
    }
	//OvLog(L"Hook before @ IDXGISwapChain_Present");
    HRESULT hr = HkIDXGISwapChain_Present(SwapChain, SyncInterval, Flags);
	//OvLog(L"Hook after @ IDXGISwapChain_Present");
	return hr;
}
typedef HRESULT(__fastcall* tPresent)(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
tPresent oPresent = nullptr;
HRESULT __fastcall hkPresent(IDXGISwapChain* pThis, void* notused, UINT SyncInterval, UINT Flags)
{
	return oPresent(pThis, SyncInterval, Flags);
}

/*HRESULT __stdcall IDXGISwapChain_Present1Hook(
	IDXGISwapChain* SwapChain,
	UINT SyncInterval,
	UINT Flags,
	const void* pPresentParameters
	)
{
	static BOOLEAN init = FALSE;

	if (!init)
	{
		init = TRUE;

		OvLog(L"Hook called @ IDXGISwapChain_Present1");
	}

	if (HkIDXGISwapChain_PresentCallback)
	{
		HkIDXGISwapChain_PresentCallback(SwapChain);
	}

	return _Present1(SwapChain, SyncInterval, Flags, pPresentParameters);
}*/


HRESULT __stdcall IDXGISwapChain_ResizeBuffersHook( 
    IDXGISwapChain *SwapChain, 
    UINT BufferCount, 
    UINT Width, 
    UINT Height, 
    DXGI_FORMAT NewFormat, 
    UINT SwapChainFlags 
    )
{
    if (HkIDXGISwapChain_ResizeBuffersCallback)
    {
        //HkIDXGISwapChain_ResizeBuffersCallback(SwapChain);
    }

    return HkIDXGISwapChain_ResizeBuffers(
        SwapChain,
        BufferCount,
        Width,
        Height,
        NewFormat,
        SwapChainFlags
        );
}


LRESULT 
CALLBACK 
FakeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


IDXGISwapChain* BuildSwapChain( HWND WindowHandle )
{
	IDXGIFactory* factory;
	IDXGIAdapter *pAdapter;
	IDXGISwapChain* swapChain;
	ID3D10Device *pDevice;
	UINT CreateFlags = 0;
	DXGI_MODE_DESC requestedMode;
	DXGI_SWAP_CHAIN_DESC scDesc;
	HRESULT hr;

	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	factory->EnumAdapters(0, &pAdapter);

	D3D10CreateDevice(
		pAdapter,
		D3D10_DRIVER_TYPE_HARDWARE,
		NULL,
		CreateFlags,
		D3D10_SDK_VERSION,
		&pDevice
		);

	requestedMode.Width = 500;
	requestedMode.Height = 500;
	requestedMode.RefreshRate.Numerator = 0;
	requestedMode.RefreshRate.Denominator = 0;
	requestedMode.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	requestedMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	requestedMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	if (!WindowHandle)
	{
		OvLog(L"Invalid window handle");
		return NULL;
	}

	// Now create the thing

	scDesc.BufferDesc = requestedMode;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = 2;
	scDesc.OutputWindow = WindowHandle;
	scDesc.Windowed = TRUE;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	hr = factory->CreateSwapChain(pDevice, &scDesc, &swapChain);

	if (FAILED(hr))
	{
		OvLog(L"IDXGIFactory::CreateSwapChain failed! hr=0x%X", hr);
		return NULL;
	}

    return swapChain;
}


DETOUR_PROPERTIES DetourDXGIPresent;
DETOUR_PROPERTIES DetourDXGIResizeBuffers;
DETOUR_PROPERTIES DetourCreateFactory0;
DETOUR_PROPERTIES DetourCreateFactory1;
DETOUR_PROPERTIES DetourCreateSwapChain0;
DETOUR_PROPERTIES DetourCreateSwapChain1;

typedef HRESULT(__stdcall *TYPE_CreateDXGIFactory)(REFIID, void**);
typedef HRESULT(__stdcall *TYPE_CreateSwapChain)(
	IDXGIFactory1                  *factory,
	IUnknown                        *pDevice,
	DXGI_SWAP_CHAIN_DESC            *pDesc,
	IDXGISwapChain                  **ppSwapChain
	);


TYPE_CreateDXGIFactory	_CreateDXGIFactory0;
TYPE_CreateDXGIFactory	_CreateDXGIFactory1;
TYPE_CreateSwapChain	_CreateSwapChain;
BOOL Factory_CreateSwapChainHooked;


HRESULT __stdcall CreateSwapChain0_hook(
	IDXGIFactory1                   *factory,
	IUnknown                        *pDevice,
	DXGI_SWAP_CHAIN_DESC            *pDesc,
	IDXGISwapChain                  **ppSwapChain
	) 
{
	VOID **vmt;
	IDXGISwapChain* swapChain;
	
	OvLog(L"CreateSwapChain!");

	HRESULT res = _CreateSwapChain(factory, pDevice, pDesc, ppSwapChain);

	if (FAILED(res)) {
		OvLog(L"Swap chain creation failed, bailing out");
		return res;
	}

	swapChain = *ppSwapChain;

	if (!swapChain)
		return res;

	vmt = (VOID**) swapChain;
	vmt = (VOID**) vmt[0];

	if (HkIDXGISwapChain_Present)
		return res;

#ifdef _M_IX86
	DetourCreate(vmt[8], IDXGISwapChain_PresentHook, &DetourDXGIPresent);
	HkIDXGISwapChain_Present = (TYPE_IDXGISwapChain_Present0)DetourDXGIPresent.Trampoline;
#else
	DetourCreate(vmt[8], hkPresent, &DetourDXGIPresent);
	oPresent = (tPresent)DetourDXGIPresent.Trampoline;
#endif

	DetourCreate(vmt[13], IDXGISwapChain_ResizeBuffersHook, &DetourDXGIResizeBuffers);
	HkIDXGISwapChain_ResizeBuffers = (TYPE_IDXGISwapChain_ResizeBuffers)DetourDXGIResizeBuffers.Trampoline;

	return res;
}


/*HRESULT __stdcall CreateSwapChain1_hook(
	IDXGIFactory1                   *factory,
	IUnknown                        *pDevice,
	DXGI_SWAP_CHAIN_DESC            *pDesc,
	IDXGISwapChain                  **ppSwapChain
	)
{
	VOID **vmt;
	IDXGISwapChain* swapChain;
	swapChain = *ppSwapChain;

	OvLog(L"CreateSwapChain1");

	HRESULT res = _CreateSwapChain(factory, pDevice, pDesc, ppSwapChain);

	if (FAILED(res)) {
		OvLog(L"Swap chain creation failed, bailing out");
		return res;
	}

	if (!swapChain)
		return res;

	vmt = (VOID**)swapChain;
	vmt = (VOID**)vmt[0];

	if (HkIDXGISwapChain_Present)
		return res;

	DetourCreate(vmt[22], IDXGISwapChain_Present1Hook, &DetourDXGIPresent);
	_Present1 = (TYPE_IDXGISwapChain_Present1)DetourDXGIPresent.Trampoline;

	DetourCreate(vmt[13], IDXGISwapChain_ResizeBuffersHook, &DetourDXGIResizeBuffers);
	HkIDXGISwapChain_ResizeBuffers = (TYPE_IDXGISwapChain_ResizeBuffers)DetourDXGIResizeBuffers.Trampoline;

	return res;
}*/


HRESULT __stdcall CreateDXGIFactory0_hook( REFIID riid, void** ppFactory ) 
{
	OvLog(L"CreateDXGIFactory called");

	HRESULT res = _CreateDXGIFactory0(riid, ppFactory);

	if (SUCCEEDED(res)) 
	{
		VOID **vmt;
		IDXGIFactory* factory = (IDXGIFactory *)*ppFactory;

		if (!factory)
			return res;

		vmt = (VOID**)factory;
		vmt = (VOID**)vmt[0];

		if (!Factory_CreateSwapChainHooked)
		{
			DetourCreate(vmt[10], CreateSwapChain0_hook, &DetourCreateSwapChain0);
			_CreateSwapChain = (TYPE_CreateSwapChain)DetourCreateSwapChain0.Trampoline;

			Factory_CreateSwapChainHooked = TRUE;
		}
	}
	else 
	{
		OvLog(L"CreateDXGIFactory failed!");
	}

	return res;
}


HRESULT __stdcall CreateDXGIFactory1_hook(REFIID riid, void** ppFactory) 
{
	OvLog(L"CreateDXGIFactory1 called");

	HRESULT res = _CreateDXGIFactory1(riid, ppFactory);

	if (SUCCEEDED(res))
	{
		VOID **vmt;
		IDXGIFactory* factory = (IDXGIFactory *)*ppFactory;

		if (!factory)
			return res;

		vmt = (VOID**)factory;
		vmt = (VOID**)vmt[0];

		if (!Factory_CreateSwapChainHooked)
		{
			DetourCreate(vmt[10], CreateSwapChain0_hook, &DetourCreateSwapChain1);
			_CreateSwapChain = (TYPE_CreateSwapChain)DetourCreateSwapChain1.Trampoline;

			Factory_CreateSwapChainHooked = TRUE;
		}
	}
	else
	{
		OvLog(L"CreateDXGIFactory failed!");
	}

	return res;
}


void CaptureInterface()
{
	HMODULE module = LoadLibraryW(L"dxgi.dll");
	void *fac0 = GetProcAddress(module, "CreateDXGIFactory");
	void *fac1 = GetProcAddress(module, "CreateDXGIFactory1");

	DetourCreate(fac0, CreateDXGIFactory0_hook, &DetourCreateFactory0);
	_CreateDXGIFactory0 = (TYPE_CreateDXGIFactory)DetourCreateFactory0.Trampoline;

	DetourCreate(fac1, CreateDXGIFactory1_hook, &DetourCreateFactory1);
	_CreateDXGIFactory1 = (TYPE_CreateDXGIFactory)DetourCreateFactory1.Trampoline;
}

void ReplaceVirtualMethod(void **VTable, int Function, void *Detour);

void BuildInterface( HWND WindowHandle )
{
	IDXGISwapChain* swapChain;
	VOID **vmt;

	swapChain = BuildSwapChain(WindowHandle);

	if (!swapChain)
	return;

	vmt = (VOID**) swapChain;
	vmt = (VOID**) vmt[0];

	//OvLog(L"Present: 0x%X", vmt[8]);

	if (!HkIDXGISwapChain_Present)
	{
		if (OvHookMethod == HOOK_METHOD_VMT)
		{
			HkIDXGISwapChain_Present = (TYPE_IDXGISwapChain_Present0)vmt[8];
			ReplaceVirtualMethod(vmt, 8, IDXGISwapChain_PresentHook);
		}
		else if (OvHookMethod == HOOK_METHOD_DETOUR)
		{
			DetourCreate(vmt[8], IDXGISwapChain_PresentHook, &DetourDXGIPresent);
			HkIDXGISwapChain_Present = (TYPE_IDXGISwapChain_Present0)DetourDXGIPresent.Trampoline;

			/*DetourCreate(vmt[13], IDXGISwapChain_ResizeBuffersHook, &DetourDXGIResizeBuffers);
			HkIDXGISwapChain_ResizeBuffers = (TYPE_IDXGISwapChain_ResizeBuffers)DetourDXGIResizeBuffers.Trampoline;*/
		}
	}

	swapChain->Release();
}


VOID DxgiHook_Initialize( IDXGISWAPCHAIN_HOOK* HookParameters )
{
	HkIDXGISwapChain_PresentCallback = (HK_IDXGISWAPCHAIN_CALLBACK)HookParameters->PresentCallback;
	HkIDXGISwapChain_ResizeBuffersCallback = (HK_IDXGISWAPCHAIN_CALLBACK)HookParameters->ResizeBuffersCallback;
	
	BuildInterface(HookParameters->WindowHandle);
	//CaptureInterface();
}


VOID DxgiHook_Destroy()
{
    if (DetourDXGIPresent.Trampoline)
        DetourDestroy(&DetourDXGIPresent);

    if (DetourDXGIResizeBuffers.Trampoline)
        DetourDestroy(&DetourDXGIResizeBuffers);
}