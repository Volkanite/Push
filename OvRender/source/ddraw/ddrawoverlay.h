#include <OvRender.h>


class DDrawOverlay : public OvOverlay
{
public:
	DDrawOverlay(OV_RENDER RenderFunction);

	// Standard functions
	VOID DrawText(WCHAR* Text);
	VOID DrawText(WCHAR* Text, DWORD Color);
	VOID DrawText(WCHAR* Text, int X, int Y, DWORD Color);
	VOID Begin();
	VOID End();
	VOID* GetDevice();
};