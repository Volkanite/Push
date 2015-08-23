#define GROUP 1
#define ITEM  2
#define HEIGHT 16


struct MenuItems
{
    WCHAR* Title, ** Options;
    int* Var, MaxValue, Type;
    DWORD* Id;
};


struct MenuSettings
{
    int DrawX, DrawY, MaxItems, SeletedIndex;
    bool Show;
};


struct MenuVars
{
    int Var;
    DWORD Id;
};


class SlOverlayMenu
{
public:
    MenuItems Items[50];
    MenuSettings mSet;
    int OpX;

    void AddGroup(WCHAR* title, WCHAR** Options, MenuVars* Variables)
    {
        AddItemToMenu(title, Options, Variables, 2, GROUP);
    }

    void AddItem(WCHAR* Title, WCHAR** Options, MenuVars* Variables, DWORD Id = 0, int MaxValue = 2)
    {
        Variables->Id = Id;

        AddItemToMenu(Title, Options, Variables, MaxValue, ITEM);
    }

    SlOverlayMenu( int OptionsX );
    void Render( int X, int Y, OvOverlay* Overlay );
    void AddItemToMenu(WCHAR* Title, WCHAR** Options, MenuVars* Variables, int MaxValue, int Type);
};