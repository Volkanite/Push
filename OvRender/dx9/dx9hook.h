

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


