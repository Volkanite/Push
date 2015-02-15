#include <sl.h>
#include <wchar.h>
#include <pushbase.h>

#include "game.h"
#include "push.h"
#include <slini.h>


#define GAME_INSTALL_PATH L"InstallPath"


WCHAR*
HeapedString( WCHAR* String )
{
    WCHAR *buffer;

    buffer = (WCHAR*) RtlAllocateHeap( PushHeapHandle, 0, (SlStringGetLength(String) + 1) * sizeof(WCHAR) );
    
    wcscpy(buffer, String);

    return buffer;
}


VOID
Game_Initialize( WCHAR* Win32Name, PUSH_GAME* Game )
{
    WCHAR *gameId, *buffer;

    Game->Win32Name = HeapedString(Win32Name);
    gameId = SlIniReadString(L"Games", Game->Win32Name, NULL, L".\\" PUSH_SETTINGS_FILE);
    
    buffer = SlIniReadSubKey(L"Game Settings", gameId, L"Name", L".\\" PUSH_SETTINGS_FILE);
    Game->Name = HeapedString(buffer);

    buffer = SlIniReadSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, L".\\" PUSH_SETTINGS_FILE);
    Game->InstallPath = HeapedString(buffer);

    // Game Settings.

    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"UseRamDisk", FALSE, L".\\" PUSH_SETTINGS_FILE))
        Game->GameSettings.UseRamDisk = TRUE;

    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"DisableRepeatKeys", FALSE, L".\\" PUSH_SETTINGS_FILE))
        Game->GameSettings.DisableRepeatKeys = TRUE;

    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"SwapWASD", FALSE, L".\\" PUSH_SETTINGS_FILE))
        Game->GameSettings.SwapWASD = TRUE;

    buffer = SlIniReadSubKey(L"Game Settings", gameId, L"ForceVsync", L".\\" PUSH_SETTINGS_FILE);
    
    if (wcscmp(buffer, L"FORCE_ON") == 0)
        Game->GameSettings.VsyncOverrideMode = PUSH_VSYNC_FORCE_ON;
    else if (wcscmp(buffer, L"FORCE_OFF") == 0)
        Game->GameSettings.VsyncOverrideMode = PUSH_VSYNC_FORCE_OFF;
}


VOID
Game_SetName( PUSH_GAME* Game, WCHAR* Name )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", Game->Win32Name, NULL, L".\\" PUSH_SETTINGS_FILE);

    SlIniWriteSubKey(L"Game Settings", gameId, L"Name", Name, L".\\" PUSH_SETTINGS_FILE);
}


VOID
Game_SetInstallPath( PUSH_GAME *Game, WCHAR* Path )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", Game->Win32Name, NULL, L".\\" PUSH_SETTINGS_FILE);

    SlIniWriteSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, Path, L".\\" PUSH_SETTINGS_FILE);
}


VOID
Game_SetFlags( PUSH_GAME *Game, DWORD Flags )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", Game->Win32Name, NULL, L".\\" PUSH_SETTINGS_FILE);

    if (Flags & GAME_RAMDISK)
        SlIniWriteSubKey(L"Game Settings", gameId, L"UseRamDisk", L"True", L".\\" PUSH_SETTINGS_FILE);
}