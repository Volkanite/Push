#include <windows.h>
#include <OvRender.h>
#include <sloverlay.h>
#include <overlay.h>


SlOverlayMenu* MnuMenu;

MenuVars MnuOsd[20];
MenuVars MnuCache[10];

WCHAR* GroupOpt[] = {L"", L""};
WCHAR* ItemOpt[] = {L"Off", L"On"};

BOOLEAN MnuInitialized;


//Add menu items to menu

VOID
AddItems()
{
    MnuMenu->AddGroup(L"OSD", GroupOpt, &MnuOsd[0].Var);

    if (MnuOsd[0].Var)
    {
        MnuMenu->AddItem(L"GPU Core utilization",    ItemOpt, &MnuOsd[1].Var);
        MnuMenu->AddItem(L"GPU Core temperature",    ItemOpt, &MnuOsd[2].Var);
        MnuMenu->AddItem(L"GPU Engine Clock",        ItemOpt, &MnuOsd[3].Var);
        MnuMenu->AddItem(L"GPU Memory Clock",        ItemOpt, &MnuOsd[4].Var);
        MnuMenu->AddItem(L"GPU VRAM usage",          ItemOpt, &MnuOsd[5].Var);
        MnuMenu->AddItem(L"CPU utilization",         ItemOpt, &MnuOsd[6].Var);
        MnuMenu->AddItem(L"CPU temperature",         ItemOpt, &MnuOsd[7].Var);
        MnuMenu->AddItem(L"RAM usage",               ItemOpt, &MnuOsd[8].Var);
        MnuMenu->AddItem(L"Max core usage",          ItemOpt, &MnuOsd[9].Var);
        MnuMenu->AddItem(L"Max thread usage",        ItemOpt, &MnuOsd[10].Var);
        MnuMenu->AddItem(L"Estimated CPU usage",     ItemOpt, &MnuOsd[11].Var);
        MnuMenu->AddItem(L"Disk read-write rate",    ItemOpt, &MnuOsd[12].Var);
        MnuMenu->AddItem(L"Disk response time",      ItemOpt, &MnuOsd[13].Var);
        MnuMenu->AddItem(L"Frame Buffer count",      ItemOpt, &MnuOsd[14].Var);
        MnuMenu->AddItem(L"Show Time",               ItemOpt, &MnuOsd[15].Var);
    }

    MnuMenu->AddGroup(L"CACHE", GroupOpt, &MnuCache[0].Var);

    if (MnuCache[0].Var)
    {
        MnuMenu->AddItem(L"NULL", ItemOpt, &MnuCache[1].Var);
        MnuMenu->AddItem(L"NULL", ItemOpt, &MnuCache[2].Var);
        MnuMenu->AddItem(L"NULL", ItemOpt, &MnuCache[3].Var);
        MnuMenu->AddItem(L"NULL", ItemOpt, &MnuCache[4].Var);
    }
}


VOID
ProcessOptions()
{
    if (MnuOsd[1].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_GPU_LOAD;
    /*else
        PushSharedMemory->OSDFlags &= ~OSD_GPU_LOAD;*/
    //this needs to be fixed, if it was enable by main gui
    //checkbox then it'll get disabled. too lazy...

    if (MnuOsd[2].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_GPU_TEMP;

    if (MnuOsd[3].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;

    if (MnuOsd[4].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;

    if (MnuOsd[7].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_CPU_TEMP;
        
    if (MnuOsd[9].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_MCU;

    if (MnuOsd[10].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_MTU;

    if (MnuOsd[14].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_TIME;
}


VOID
MnuRender( OvOverlay* Overlay )
{
    if (!MnuInitialized)
    {
        MnuMenu = new SlOverlayMenu(300);
        MnuInitialized = TRUE;
    }

    if( MnuMenu->mSet.MaxItems == 0 )
        AddItems();

    //Call drawing and navigation functions
    MnuMenu->Render(100, 200, Overlay);
    ProcessOptions();
}


#define GROUP 1
#define ITEM  2
#define HEIGHT 16


DWORD LightBlue = 0xFF4DD0EB;
DWORD Green     = 0xFF33FF00;
DWORD White     = 0xFFE6E6E6;
DWORD Blue      = 0xFF00A4C5;

WNDPROC         OldWNDPROC;
SlOverlayMenu*  OvmMenu;


VOID
MenuKeyboardCallback( WORD Key )
{
    switch(Key)
    {
        case VK_INSERT:
            OvmMenu->mSet.Show = !OvmMenu->mSet.Show;
            break;
        
        case VK_UP:
            {
                if (!OvmMenu->mSet.Show)
                    break;

                OvmMenu->mSet.SeletedIndex--;
                
                if (OvmMenu->mSet.SeletedIndex < 0)
                    OvmMenu->mSet.SeletedIndex = OvmMenu->mSet.MaxItems - 1;

            } break;

        case VK_DOWN:
            {
                if (!OvmMenu->mSet.Show)
                    break;

                OvmMenu->mSet.SeletedIndex++;
                
                if (OvmMenu->mSet.SeletedIndex == OvmMenu->mSet.MaxItems)
                    OvmMenu->mSet.SeletedIndex = 0;

            } break;

        case VK_LEFT:
            {
                if (!OvmMenu->mSet.Show)
                    break;

                if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var && *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var > 0)
                {
                    *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var += -1;
                    
                    if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == GROUP)
                        OvmMenu->mSet.MaxItems = 0;
                }

            } break;

        case VK_RIGHT:
            {
                if (!OvmMenu->mSet.Show)
                    break;

                if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var 
                    && *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var < (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].MaxValue - 1))
                {
                    *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var += 1;

                    if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == GROUP)
                        OvmMenu->mSet.MaxItems = 0;
                }

            } break;
    }
}


SlOverlayMenu::SlOverlayMenu( int OptionsX )
{
    OpX = OptionsX;
    
    mSet.Show = FALSE;
    mSet.MaxItems = 0;
    mSet.SeletedIndex = 0;
    
    OvmMenu = this;
}


void SlOverlayMenu::AddItemToMenu(WCHAR* Title, WCHAR** Options, int* Var, int MaxValue, int Type)
{
    Items[mSet.MaxItems].Title = Title;
    Items[mSet.MaxItems].Options= Options;
    Items[mSet.MaxItems].Var = Var;
    Items[mSet.MaxItems].MaxValue = MaxValue;
    Items[mSet.MaxItems].Type = Type;
    mSet.MaxItems++;
}


VOID 
SlOverlayMenu::Render( int X, int Y, OvOverlay* Overlay )
{
    DWORD ColorOne, ColorTwo;
    int ValueOne, ValueTwo;

    if (!mSet.Show)
        return;

    for (int i = 0; i < mSet.MaxItems; i++)
    {
        ValueOne = (Items[i].Var) ? (*Items[i].Var) : 0;
        ValueTwo = (Items[i].Var) ? (*Items[i].Var) : 0;

        if (i == mSet.SeletedIndex)
        {
            ColorOne = LightBlue;
            ColorTwo = (ValueTwo) ? Green : White;
        }
        else if (Items[i].Type == GROUP)
        {
            ColorOne = Blue;
            ColorTwo = Blue;
        }
        else
        {
            ColorOne = (ValueOne) ? White : White;
            ColorTwo = (ValueTwo) ? Green : White;
        }

        if (Items[i].Type == GROUP)
            Overlay->DrawText(Items[i].Title, X, Y, ColorOne);

        if (Items[i].Type == ITEM)
            Overlay->DrawText(Items[i].Title, X + 20, Y, ColorOne);

        if (Items[i].Options)
            Overlay->DrawText(Items[i].Options[ValueTwo], OpX, Y, ColorTwo);

        Y += HEIGHT;
    }
}
