#include <sl.h>
#include <wchar.h>
#include <pushbase.h>

#include "game.h"
#include "push.h"
#include <slini.h>


#define GAME_INSTALL_PATH L"InstallPath"


WCHAR* HeapedString( WCHAR* String )
{
    WCHAR *buffer;

    buffer = (WCHAR*) Memory_Allocate( (String_GetLength(String) + 1) * sizeof(WCHAR) );

    String_Copy(buffer, String);

    return buffer;
}


VOID Game_Initialize(WCHAR* Win32Name, PUSH_GAME* Game)
{
    WCHAR gameId[260];
    WCHAR *buffer;
    WCHAR *lastSlash;
    WCHAR testing[260];

    String_Format(testing, 260, L"Game_Initialize(%s)", Win32Name);
    Log(testing);
    Game->ExecutablePath = HeapedString(Win32Name);
    lastSlash = String_FindLastChar(Game->ExecutablePath, '\\');
    Game->ExecutableName = lastSlash + 1;

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260, L".\\" PUSH_SETTINGS_FILE);

    String_CopyN(Game->Id, gameId, 2);

    buffer = SlIniReadSubKey(L"Game Settings", gameId, L"Name", L".\\" PUSH_SETTINGS_FILE);
    Game->Name = HeapedString(buffer ? buffer : Game->ExecutableName);

    buffer = SlIniReadSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, L".\\" PUSH_SETTINGS_FILE);
    Game->InstallPath = HeapedString(buffer);

    buffer = SlIniReadSubKey(L"Game Settings", gameId, L"CheckSum", L".\\" PUSH_SETTINGS_FILE);
    if (buffer) Game->CheckSum = wcstol(buffer, NULL, 16);

    // Game Settings.

    if (SlIniReadSubKeyBoolean(
        L"Game Settings",
        gameId,
        L"DisableOverlay",
        FALSE,
        L".\\" PUSH_SETTINGS_FILE)
        )
    {
        Game->Settings.DisableOverlay = TRUE;
    }

    if (SlIniReadSubKeyBoolean(
        L"Game Settings",
        gameId,
        L"UseRamDisk",
        FALSE,
        L".\\" PUSH_SETTINGS_FILE)
        )
    {
        Game->Settings.UseRamDisk = TRUE;
    }

    if (SlIniReadSubKeyBoolean(
        L"Game Settings",
        gameId,
        L"DisableRepeatKeys",
        FALSE,
        L".\\" PUSH_SETTINGS_FILE)
        )
    {
        Game->Settings.DisableRepeatKeys = TRUE;
    }

    if (SlIniReadSubKeyBoolean(
        L"Game Settings",
        gameId,
        L"SwapWASD",
        FALSE,
        L".\\" PUSH_SETTINGS_FILE)
        )
    {
        Game->Settings.SwapWASD = TRUE;
    }

    if (SlIniReadSubKeyBoolean(
        L"Game Settings",
        gameId,
        L"ForceMaxClocks",
        FALSE,
        L".\\" PUSH_SETTINGS_FILE)
        )
    {
        Game->Settings.ForceMaxClocks = TRUE;
    }

    buffer = SlIniReadSubKey(L"Game Settings", gameId, L"ForceVsync", L".\\" PUSH_SETTINGS_FILE);

    if (String_Compare(buffer, L"FORCE_ON") == 0)
    {
        Game->Settings.VsyncOverrideMode = PUSH_VSYNC_FORCE_ON;
        OutputDebugStringW(L"PUSH_VSYNC_FORCE_ON.1");
    }
        
    else if (String_Compare(buffer, L"FORCE_OFF") == 0)
        Game->Settings.VsyncOverrideMode = PUSH_VSYNC_FORCE_OFF;
}


VOID Game_SetName( PUSH_GAME* Game, WCHAR* Name )
{
    WCHAR gameId[260];

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260, L".\\" PUSH_SETTINGS_FILE);

    SlIniWriteSubKey(L"Game Settings", gameId, L"Name", Name, L".\\" PUSH_SETTINGS_FILE);
}


VOID Game_SetInstallPath( PUSH_GAME *Game, WCHAR* Path )
{
    WCHAR gameId[260];

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260, L".\\" PUSH_SETTINGS_FILE);

    SlIniWriteSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, Path, L".\\" PUSH_SETTINGS_FILE);
}


VOID Game_SetFlags( PUSH_GAME *Game, DWORD Flags )
{
    WCHAR gameId[260];

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260, L".\\" PUSH_SETTINGS_FILE);

    if (Flags & GAME_RAMDISK)
        SlIniWriteSubKey(L"Game Settings", gameId, L"UseRamDisk", L"True", L".\\" PUSH_SETTINGS_FILE);
}


VOID Game_SetCheckSum( PUSH_GAME* Game, DWORD CheckSum )
{
    WCHAR gameId[260];
    WCHAR checkSum[100];

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260, L".\\" PUSH_SETTINGS_FILE);

    swprintf(checkSum, 100, L"0x%X", CheckSum);
    SlIniWriteSubKey(L"Game Settings", gameId, L"CheckSum", checkSum, L".\\" PUSH_SETTINGS_FILE);
}


GAME_LIST Game_GetGames()
{
    static GAME_LIST_ENTRY* firstEntry = NULL;
    GAME_LIST_ENTRY* gameListEntry = NULL;
    WCHAR *games;
    INT32 i;

    if (firstEntry)
        return firstEntry;

    games = Memory_Allocate(512 * sizeof(WCHAR));

    Ini_GetString(L"Games", NULL, NULL, games, 512, L".\\" PUSH_SETTINGS_FILE);

    if (games)
    {
        for (i = 0; games[0] != '\0'; i++)
        {
            PUSH_GAME* game;
            GAME_LIST_ENTRY* newEntry;

            game = (PUSH_GAME*) Memory_Allocate(sizeof(PUSH_GAME));
            newEntry = (GAME_LIST_ENTRY*) Memory_Allocate(sizeof(GAME_LIST_ENTRY));

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

            games = String_FindLastChar(games, '\0') + 1;
        }
    }

    return firstEntry;
}
