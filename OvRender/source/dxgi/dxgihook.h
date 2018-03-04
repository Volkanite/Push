typedef struct _IDXGISWAPCHAIN_HOOK
{
    VOID* PresentCallback;
    VOID* ResizeBuffersCallback;

}IDXGISWAPCHAIN_HOOK;

VOID DxgiHook_Initialize(IDXGISWAPCHAIN_HOOK* HookParameters);
VOID DxgiHook_Destroy();