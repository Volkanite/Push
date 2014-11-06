OvRender
========

Unified renderer. Render in any games frame buffer.

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

Credits
-------

DetourXS (https://github.com/DominicTobias/detourxs)
