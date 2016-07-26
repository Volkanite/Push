/*typedef VOID (*HK_IDXGISWAPCHAIN_PRESENT_CALLBACK)(
    IDXGISwapChain* SwapChain
    );*/

typedef struct _IDXGISWAPCHAIN_HOOK
{
    VOID* PresentCallback;
    VOID* ResizeBuffersCallback;

}IDXGISWAPCHAIN_HOOK;

VOID HookDxgi( IDXGISWAPCHAIN_HOOK* HookParameters );