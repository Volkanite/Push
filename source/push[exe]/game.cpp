#include <sl.h>
#include <wchar.h>

#include "game.h"
#include "push.h"
#include <slini.h>


#define GAME_INSTALL_PATH L"InstallPath"


PushGame::PushGame( WCHAR* Game )
{
    // Allocate some memory for the game name
    GameWin32Name = (WCHAR*) RtlAllocateHeap(
        PushHeapHandle,
        0,
        (SlStringGetLength(Game) + 1) * sizeof(WCHAR)
        );

    // Save new batchfile name
    wcscpy(GameWin32Name, Game);

}


WCHAR*
PushGame::GetName()
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", GameWin32Name, NULL, L".\\" PUSH_SETTINGS_FILE);
    
    return SlIniReadSubKey(L"Game Settings", gameId, L"Name", L".\\" PUSH_SETTINGS_FILE);
}


VOID
PushGame::SetName( WCHAR* Name )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", GameWin32Name, NULL, L".\\" PUSH_SETTINGS_FILE);

    SlIniWriteSubKey(L"Game Settings", gameId, L"Name", Name, L".\\" PUSH_SETTINGS_FILE);
}


WCHAR*
PushGame::GetInstallPath()
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", GameWin32Name, NULL, L".\\" PUSH_SETTINGS_FILE);

    return SlIniReadSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, L".\\" PUSH_SETTINGS_FILE);
}


VOID
PushGame::SetInstallPath( WCHAR* Path )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", GameWin32Name, NULL, L".\\" PUSH_SETTINGS_FILE);

    SlIniWriteSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, Path, L".\\" PUSH_SETTINGS_FILE);
}


DWORD
PushGame::GetFlags()
{
    DWORD flags = 0;
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", GameWin32Name, NULL, L".\\" PUSH_SETTINGS_FILE);

    // Need ramdisk?
    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"UseRamDisk", FALSE, L".\\" PUSH_SETTINGS_FILE))
        flags |= GAME_RAMDISK;

    //check for forced vsync
    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"ForceVsync", FALSE, L".\\" PUSH_SETTINGS_FILE))
        flags |= GAME_VSYNC;

    //check for key repeat
    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"DisableRepeatKeys", FALSE, L".\\" PUSH_SETTINGS_FILE))
        flags |= GAME_REPEAT_KEYS;

    // Check if user wants to emulate arrow keys with WASD keys
    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"SwapWASD", FALSE, L".\\" PUSH_SETTINGS_FILE))
        flags |= GAME_WASD;

    return flags;
}


VOID
PushGame::SetFlags( DWORD Flags )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", GameWin32Name, NULL, L".\\" PUSH_SETTINGS_FILE);

    if (Flags & GAME_RAMDISK)
        SlIniWriteSubKey(L"Game Settings", gameId, L"UseRamDisk", L"True", L".\\" PUSH_SETTINGS_FILE);
}