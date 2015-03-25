OvRender
========

Unified renderer. Render in any games' frame buffer. Supports DirectX 8/9/10/11 games. OpenGL is not supported but can be easily implemented.

Example
-------

    #include <Windows.h>
    #include <OvRender.h>
    
    #pragma comment(lib, "OvRender.lib")
    
    
    VOID Render( OvOverlay* Overlay )
    {
        Overlay->DrawText(L"u can draw anything in this render loop!\n");
    }
    
    
    BOOL __stdcall DllMain( HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpReserved )
    {
        if (fdwReason == DLL_PROCESS_ATTACH)
            OvCreateOverlay( Render );
            
        return TRUE;
    }
    
Requirements
------------

DirectX SDK (June 2010)

Credits
-------

DetourXS (https://github.com/DominicTobias/detourxs)
