#define GROUP 1
#define ITEM  2
#define HEIGHT 16


struct MenuItems
{
	WCHAR* Title, ** Options;
	int* Var, MaxValue, Type;
};


struct MenuSettings
{
	int DrawX, DrawY, MaxItems, SeletedIndex;
	bool Show;
};


struct MenuVars
{
	int Var;
};


class SlOverlayMenu
{
public:
	MenuItems Items[50];
	MenuSettings mSet;
	int OpX;

	void AddGroup(WCHAR* title, WCHAR** Options, int* Var)
	{
		AddItemToMenu(title, Options, Var, 2, GROUP);
	}

	void AddItem(WCHAR* Title, WCHAR** Options, int* Var, int MaxValue = 2)
	{
		AddItemToMenu(Title, Options, Var, MaxValue, ITEM);
	}

	SlOverlayMenu( int OptionsX );
	void Navigate();
	void Render( int X, int Y, OvOverlay* Overlay );
	void AddItemToMenu(WCHAR* Title, WCHAR** Options, int* Var, int MaxValue, int Type);
};