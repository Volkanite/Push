

#ifdef __cplusplus
extern "C" {
#endif

VOID IDirect3D_CreateDeviceCallback(
    D3DPRESENT_PARAMETERS* presentationParameters
    );

VOID IDirect3DDevice9_PresentCallback(
    IDirect3DDevice9* OBJECT_IDirect3DDevice9
    );

VOID IDirect3DSwapChain9_PresentCallback(
    IDirect3DDevice9* OBJECT_IDirect3DDevice9
    );

VOID IDirect3DDevice9_ResetCallback(
    D3DPRESENT_PARAMETERS* presentationParameters
    );

#ifdef __cplusplus
}
#endif
typedef VOID (*DX9HOOK_PRESENT_CALLBACK) (
    IDirect3DDevice9* OBJECT_IDirect3DDevice9
    );

typedef VOID (*DX9HOOK_RESET_CALLBACK)(
    D3DPRESENT_PARAMETERS* PresentationParameters
    );


typedef struct _DX9HOOK_PARAMS
{
    DX9HOOK_PRESENT_CALLBACK    PresentCallback;
    DX9HOOK_RESET_CALLBACK      ResetCallback;
    DX9HOOK_RESET_CALLBACK      CreateDeviceCallback;

} D3D9HOOK_PARAMS;

extern BOOLEAN D3D9Hook_ForceReset;

VOID 
Dx9Hook_Initialize( D3D9HOOK_PARAMS* HookParams );
