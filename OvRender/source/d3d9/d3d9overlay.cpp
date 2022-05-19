#include <windows.h>
#include <d3d9.h>

#include "d3d9overlay.h"
#include "d3d9font.h"
#include "d3d9hook.h"
#include <OvRender.h>


Dx9Font* D3D9Font;
extern Dx9Overlay* D3D9Overlay;
IDirect3DDevice9* Dx9OvDevice;
OV_WINDOW_MODE D3D9Hook_WindowMode;
UINT32 BackBufferCount;
BOOLEAN ResetFont;

BOOLEAN TakeScreenShot;
BOOLEAN StartRecording;
BOOLEAN StopRecording;
BOOLEAN Recording;

HRESULT MakeScreenShot(bool bHalfSize);
bool RecordFrame();
ULONG __stdcall CloseAVI(LPVOID Params);
DWORD InitializeAviFile();
bool InitFreqUnits();

//VOID OvLog(const wchar_t* Format, ...);


VOID DebugRec()
{
    D3DRECT rec = { 0, 0, 10, 10 };

    if (Dx9OvDevice)
        Dx9OvDevice->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_ARGB(225, 225, 0, 0), 0, 0);
}


VOID SetFont( WCHAR* FontName, BOOLEAN Bold, UINT32 Size )
{
    if (FontName)
        D3D9Overlay->FontProperties.Name = FontName;

    D3D9Overlay->FontProperties.Bold = Bold;
    D3D9Overlay->FontProperties.Size = Size;

    ResetFont = TRUE;
}


VOID ChangeVsync(BOOLEAN Setting)
{
    if (Setting)
        D3D9Overlay->VsyncOverrideMode = VSYNC_FORCE_ON;
    else
        D3D9Overlay->VsyncOverrideMode = VSYNC_FORCE_OFF;

    D3D9Hook_ForceReset = TRUE;
}


WCHAR* GetVsyncChar( OV_VSYNC_OVERRIDE_MODE Mode )
{
    switch (Mode)
    {
    case VSYNC_UNCHANGED:
        return L"VSYNC_UNCHANGED";
        break;
    case VSYNC_FORCE_ON:
        return L"VSYNC_FORCE_ON";
        break;
    case VSYNC_FORCE_OFF:
        return L"VSYNC_FORCE_OFF";
        break;
    default:
        return L"VYSNC_UNKNOWN";
        break;
    }
}


VOID UpdatePresentationParameters( D3DPRESENT_PARAMETERS* PresentationParameters )
{
    // Force vsync?
    OvLog(L"UpdatePresentationParameters(%s)", GetVsyncChar(D3D9Overlay->VsyncOverrideMode));

    if (D3D9Overlay->VsyncOverrideMode == VSYNC_FORCE_ON)
    {
        PresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    }
    else if (D3D9Overlay->VsyncOverrideMode == VSYNC_FORCE_OFF)
    {
        PresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    if (D3D9Hook_WindowMode == WINDOW_WINDOWED)
    {
        RECT r;

        PresentationParameters->Windowed = TRUE;
        PresentationParameters->Flags = 0;
        PresentationParameters->FullScreen_RefreshRateInHz = 0;

        SetWindowLong(
            PresentationParameters->hDeviceWindow, 
            GWL_STYLE, 
            WS_POPUP | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE
            );

        GetClientRect(PresentationParameters->hDeviceWindow, &r);

        SetWindowPos(
            PresentationParameters->hDeviceWindow, 
            HWND_NOTOPMOST, 
            0, 
            0,
            PresentationParameters->BackBufferWidth + (PresentationParameters->BackBufferWidth - r.right),
            PresentationParameters->BackBufferHeight + (PresentationParameters->BackBufferHeight - r.bottom),
            SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOMOVE
            );
    }
    else if (D3D9Hook_WindowMode == WINDOW_FULLSCREEN)
    {
        PresentationParameters->Windowed = FALSE;
    }

    D3D9Overlay->BackBufferHeight = PresentationParameters->BackBufferHeight;
    D3D9Overlay->BackBufferWidth = PresentationParameters->BackBufferWidth;
    BackBufferCount = PresentationParameters->BackBufferCount;
}


VOID Dx9Overlay_CreateDevice( D3DPRESENT_PARAMETERS* PresentationParameters )
{
    OvLog(L"Dx9Overlay_CreateDevice()");
    UpdatePresentationParameters(PresentationParameters);
}


VOID DeleteFont()
{
    if (D3D9Font)
    {
        D3D9Font->InvalidateDeviceObjects();
        D3D9Font->DeleteDeviceObjects();
    }

    D3D9Font = NULL;
}


VOID Dx9Overlay_Reset( D3DPRESENT_PARAMETERS* PresentationParameters )
{
    OvLog(L"Dx9Overlay_Reset()");

    DeleteFont();
    UpdatePresentationParameters(PresentationParameters);
}


VOID Dx9OvRender( IDirect3DDevice9* Device )
{
    IDirect3DSurface9 *renderTarget = NULL;
    IDirect3DSurface9 *backBuffer = NULL;
    D3DSURFACE_DESC backBufferDesc;
    D3DVIEWPORT9 viewport;

    if (ResetFont)
    {
        DeleteFont();
        ResetFont = FALSE;
    }

    if (D3D9Font == NULL)
    {
        D3D9Font = new Dx9Font(Device, &D3D9Overlay->FontProperties);
        D3D9Font->RestoreDeviceObjects();

        D3D9Overlay->BackBufferWidth = 0;
        D3D9Overlay->BackBufferHeight = 0;
        
        Dx9OvDevice = Device;
    }

    // Backup render target.
    
    Device->GetRenderTarget( 0, &renderTarget );
    Device->GetViewport(&viewport);

    // Set backbuffer as new render target.
    
    Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

    if (backBuffer)
        backBuffer->GetDesc(&backBufferDesc);

    //steal some info
    if (D3D9Overlay->BackBufferWidth == 0 && D3D9Overlay->BackBufferHeight == 0)
    {
        D3D9Overlay->BackBufferWidth = backBufferDesc.Width;
        D3D9Overlay->BackBufferHeight = backBufferDesc.Height;
    }

    if (backBufferDesc.Width != 1 && backBufferDesc.Height != 1)
    {
        Device->SetRenderTarget(0, backBuffer);
    }
        
    // Render our stuff.
    D3D9Overlay->Render();

    // Restore render target.

    Device->SetRenderTarget( 0, renderTarget );
    Device->SetViewport(&viewport);

    // Cleanup.
    if (backBuffer)
        backBuffer->Release();

    if (renderTarget)
        renderTarget->Release();
}


VOID Dx9Overlay_Present( IDirect3DDevice9* Device )
{
    static BOOLEAN initialized = FALSE;

    // Screenshot here (before we render) to exclude menu

    Dx9OvRender(Device);

    // Screenshot here (after we render) to include our menu

    if (TakeScreenShot)
    {
        MakeScreenShot(false);
        TakeScreenShot = false;
    }

    if (StartRecording)
    {
        StartRecording = FALSE;
        Recording = TRUE;

        InitFreqUnits();
        InitializeAviFile();
        
    }
    else if (StopRecording)
    {
        StopRecording = FALSE;
        Recording = FALSE;

        CreateThread(0, 0, &CloseAVI, 0, 0, 0);
    }

    if (Recording)
    {
        BOOLEAN result = RecordFrame();

        if (!result)
        {
            OvLog(L"Dx9Overlay_Present: Failed to record frame");
        }

        DebugRec(); //Put this after RecordFrame() to exclude it from recorded video.
    }

    if (!initialized)
    { 
        if (D3D9Overlay->VsyncOverrideMode == VSYNC_FORCE_ON 
         || D3D9Overlay->VsyncOverrideMode == VSYNC_FORCE_OFF)
        {
            D3D9Hook_ForceReset = TRUE;
        }

        initialized = TRUE;

        GraphicsApi = API_D3D9;
    }
}


Dx9Overlay::Dx9Overlay( OV_RENDER RenderFunction )
{
    D3D9HOOK_PARAMS hookParams;

    hookParams.PresentCallback = Dx9Overlay_Present;
    hookParams.ResetCallback = Dx9Overlay_Reset;
    hookParams.CreateDeviceCallback = Dx9Overlay_CreateDevice;

    OvLog(L"Dx9Overlay::Dx9Overlay( => )");

    Dx9Hook_Initialize(&hookParams);

    UserRenderFunction = RenderFunction;
}


Dx9Overlay::~Dx9Overlay()
{
    Dx9Hook_Destroy();

    /*if (D3D9Font)
        D3D9Font->~Dx9Font();*/
}


VOID
Dx9Overlay::DrawText( WCHAR* Text )
{   
    DrawText(Text, 0xFFFFFF00);
}

UINT32 dsec;
VOID
Dx9Overlay::DrawText( WCHAR* Text, DWORD Color )
{
    DrawText(Text, 20, Line, Color);
    //OvLog(L"DrawText: %s", Text);
    dsec++;
    Line += 15;
}


VOID Dx9Overlay::DrawText( WCHAR* Text, int X, int Y, DWORD Color )
{
    D3D9Font->DrawText((FLOAT)X, (FLOAT)Y, Color, Text, NULL);
}


VOID
Dx9Overlay::Begin()
{
    dsec = 0;
}


VOID
Dx9Overlay::End()
{
    //OvLog(L"dsec %u", dsec);
}


VOID*
Dx9Overlay::GetDevice()
{
    return Dx9OvDevice;
}