// Callbacks
typedef VOID (*HK_IDIRECT3DDEVICE8_PRESENT_CALLBACK)(
    IDirect3DDevice8* Device
    );

typedef VOID (*HK_IDIRECT3DDEVICE8_RESET_CALLBACK)(
    );


VOID HookD3D8( 
    HK_IDIRECT3DDEVICE8_PRESENT_CALLBACK IDirect3DDevice8_PresentCallback, 
    HK_IDIRECT3DDEVICE8_RESET_CALLBACK IDirect3DDevice8_ResetCallback
    );

VOID Dx8Hook_Destroy();