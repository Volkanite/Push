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

    SlStringCopy(buffer, String);

    return buffer;
}


VOID Game::Initialize( WCHAR* Win32Name, PUSH_GAME* Game )
{
    WCHAR *gameId;
    WCHAR *buffer;
    WCHAR *lastSlash;

    Game->ExecutablePath = HeapedString(Win32Name);
    lastSlash = SlStringFindLastChar(Game->ExecutablePath, '\\');
    Game->ExecutableName = lastSlash + 1;

    gameId = SlIniReadString(L"Games", Game->ExecutablePath, NULL, L".\\" PUSH_SETTINGS_FILE);
    SlStringCopyN(Game->Id, gameId, 2);

    buffer = SlIniReadSubKey(L"Game Settings", gameId, L"Name", L".\\" PUSH_SETTINGS_FILE);

    if (!buffer) return;

    Game->Name = HeapedString(buffer);

    buffer = SlIniReadSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, L".\\" PUSH_SETTINGS_FILE);
    Game->InstallPath = HeapedString(buffer);

    buffer = SlIniReadSubKey(L"Game Settings", gameId, L"CheckSum", L".\\" PUSH_SETTINGS_FILE);
    if (buffer) Game->CheckSum = wcstol(buffer, NULL, 16);

    // Game Settings.

    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"UseRamDisk", FALSE, L".\\" PUSH_SETTINGS_FILE))
        Game->Settings.UseRamDisk = TRUE;

    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"DisableRepeatKeys", FALSE, L".\\" PUSH_SETTINGS_FILE))
        Game->Settings.DisableRepeatKeys = TRUE;

    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"SwapWASD", FALSE, L".\\" PUSH_SETTINGS_FILE))
        Game->Settings.SwapWASD = TRUE;

    buffer = SlIniReadSubKey(L"Game Settings", gameId, L"ForceVsync", L".\\" PUSH_SETTINGS_FILE);

    if (SlStringCompare(buffer, L"FORCE_ON") == 0)
        Game->Settings.VsyncOverrideMode = PUSH_VSYNC_FORCE_ON;
    else if (SlStringCompare(buffer, L"FORCE_OFF") == 0)
        Game->Settings.VsyncOverrideMode = PUSH_VSYNC_FORCE_OFF;

    if (SlIniReadSubKeyBoolean(L"Game Settings", gameId, L"ForceMaxClocks", FALSE, L".\\" PUSH_SETTINGS_FILE))
        Game->Settings.ForceMaxClocks = TRUE;


}


VOID Game::SetName( PUSH_GAME* Game, WCHAR* Name )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", Game->ExecutablePath, NULL, L".\\" PUSH_SETTINGS_FILE);

    SlIniWriteSubKey(L"Game Settings", gameId, L"Name", Name, L".\\" PUSH_SETTINGS_FILE);
}


VOID Game::SetInstallPath( PUSH_GAME *Game, WCHAR* Path )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", Game->ExecutablePath, NULL, L".\\" PUSH_SETTINGS_FILE);

    SlIniWriteSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, Path, L".\\" PUSH_SETTINGS_FILE);
}


VOID Game::SetFlags( PUSH_GAME *Game, DWORD Flags )
{
    WCHAR* gameId;

    gameId = SlIniReadString(L"Games", Game->ExecutablePath, NULL, L".\\" PUSH_SETTINGS_FILE);

    if (Flags & GAME_RAMDISK)
        SlIniWriteSubKey(L"Game Settings", gameId, L"UseRamDisk", L"True", L".\\" PUSH_SETTINGS_FILE);
}


VOID Game::SetCheckSum( PUSH_GAME* Game, DWORD CheckSum )
{
    WCHAR* gameId;
    WCHAR checkSum[100];

    gameId = SlIniReadString(L"Games", Game->ExecutablePath, NULL, L".\\" PUSH_SETTINGS_FILE);

    swprintf(checkSum, 100, L"0x%X", CheckSum);
    SlIniWriteSubKey(L"Game Settings", gameId, L"CheckSum", checkSum, L".\\" PUSH_SETTINGS_FILE);
}


GAME_LIST Game::GetGames()
{
    static GAME_LIST_ENTRY* firstEntry = NULL;
    GAME_LIST_ENTRY* gameListEntry = NULL;
    WCHAR* games;
    INT32 i;

    if (firstEntry)
        return firstEntry;

    games = SlIniReadString(L"Games", NULL, NULL, L".\\" PUSH_SETTINGS_FILE);

    if (games)
    {
        for (i = 0; games[0] != '\0'; i++)
        {
            PUSH_GAME* game;
            GAME_LIST_ENTRY* newEntry;

            game = RtlAllocateHeap(PushHeapHandle, 0, sizeof(PUSH_GAME));
            newEntry = RtlAllocateHeap(PushHeapHandle, 0, sizeof(GAME_LIST_ENTRY));

            Game_Initialize(games, game);

            if (!gameListEntry)
            {
                gameListEntry = newEntry;
                firstEntry = gameListEntry;
            }
            else
            {
                gameListEntry->NextEntry = newEntry;
                gameListEntry = gameListEntry->NextEntry;
            }

            gameListEntry->Game = game;
            gameListEntry->NextEntry = NULL;

            games = SlStringFindLastChar(games, '\0') + 1;
        }
    }

    return firstEntry;
}
