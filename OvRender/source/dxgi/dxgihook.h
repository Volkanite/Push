typedef struct _IDXGISWAPCHAIN_HOOK
{
    VOID*	PresentCallback;
    VOID*	ResizeBuffersCallback;
	HWND	WindowHandle;

}IDXGISWAPCHAIN_HOOK;

VOID DxgiHook_Initialize(IDXGISWAPCHAIN_HOOK* HookParameters);
VOID DxgiHook_Destroy();