#include <sl.h>
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


VOID Game_Initialize( WCHAR* Win32Name, PUSH_GAME* Game )
{
    WCHAR gameId[260];
    WCHAR buffer[260];
    WCHAR installPath[260];
    WCHAR *lastSlash;
    DWORD start;
    DWORD end;
    DWORD len;

    if (!Win32Name)
        return;

    //Log(L"Game_Initialize(%s)", Win32Name);
    
    Game->ExecutablePath = HeapedString(Win32Name);
    lastSlash = String_FindLastChar(Game->ExecutablePath, '\\');
    Game->ExecutableName = lastSlash + 1;

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260);

    String_CopyN(Game->Id, gameId, 2);

    Ini_ReadSubKey(
        L"Game Settings", 
        gameId, 
        L"Name", 
        Game->ExecutableName, 
        buffer, 
        260
        );

    Game->Name = HeapedString(buffer);

    // Get install path

    // get default one by determing the path of the exectuble. The install directory
    // does not nessarilly have to be where the executable is located though, hence the
    // purpose of the Game::InstallPath field.
    
    start = Game->ExecutablePath;
    end = lastSlash;
    len = (end - start) / sizeof(WCHAR);

    String_CopyN(installPath, Game->ExecutablePath, len);
    
    installPath[len] = L'\0';
    
    Ini_ReadSubKey(
        L"Game Settings", 
        gameId, 
        GAME_INSTALL_PATH, 
        installPath, 
        buffer, 
        260
        );
    
    Game->InstallPath = HeapedString(buffer);

    // Get checksum
    Ini_ReadSubKey(L"Game Settings", gameId, L"CheckSum", NULL, buffer, 260);
    if (buffer) Game->CheckSum = wcstol(buffer, NULL, 16);

    // Game Settings.

    if (Ini_ReadSubKeyBoolean(L"Game Settings", gameId, L"DisableOverlay", FALSE))
    {
        Game->Settings.DisableOverlay = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", gameId, L"UseRamDisk", FALSE))
    {
        Game->Settings.UseRamDisk = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", gameId, L"DisableRepeatKeys", FALSE))
    {
        Game->Settings.DisableRepeatKeys = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", gameId, L"SwapWASD", FALSE))
    {
        Game->Settings.SwapWASD = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", gameId, L"ForceMaxClocks", FALSE))
    {
        Game->Settings.ForceMaxClocks = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", gameId, L"Patch", FALSE))
    {
        Game->Settings.PatchMemory = TRUE;
    }

    Ini_ReadSubKey(L"Game Settings", gameId, L"ForceVsync", NULL, buffer, 260);

    if (String_Compare(buffer, L"FORCE_ON") == 0)
    {
        Game->Settings.VsyncOverrideMode = PUSH_VSYNC_FORCE_ON;
    }
        
    else if (String_Compare(buffer, L"FORCE_OFF") == 0)
        Game->Settings.VsyncOverrideMode = PUSH_VSYNC_FORCE_OFF;
}


VOID Game_SetName( PUSH_GAME* Game, WCHAR* Name )
{
    WCHAR gameId[260];

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260);

    SlIniWriteSubKey(L"Game Settings", gameId, L"Name", Name);
}


VOID Game_SetInstallPath( PUSH_GAME *Game, WCHAR* Path )
{
    WCHAR gameId[260];

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260);

    SlIniWriteSubKey(L"Game Settings", gameId, GAME_INSTALL_PATH, Path);
}


VOID Game_SetFlags( PUSH_GAME *Game, DWORD Flags )
{
    WCHAR gameId[260];

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260);

    if (Flags & GAME_RAMDISK)
        SlIniWriteSubKey(L"Game Settings", gameId, L"UseRamDisk", L"True");
}


VOID Game_SetCheckSum( PUSH_GAME* Game, DWORD CheckSum )
{
    WCHAR gameId[260];
    WCHAR checkSum[100];

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, gameId, 260);

    String_Format(checkSum, 100, L"0x%X", CheckSum);
    SlIniWriteSubKey(L"Game Settings", gameId, L"CheckSum", checkSum);
}


GAME_LIST Game_GetGames()
{
    static GAME_LIST_ENTRY* firstEntry = NULL;
    GAME_LIST_ENTRY* gameListEntry = NULL;
    WCHAR *games;
    INT32 i;
    INT32 count = 0;
    INT32 bufferSize = 512;
    DWORD returnValue;

    if (firstEntry)
        return firstEntry;

    games = (WCHAR*)Memory_Allocate(4);

    do
    {
        count++;

        games = (WCHAR*)Memory_ReAllocate(
            games,
            bufferSize * 2 * count
            );

        returnValue = Ini_GetString(
            L"Games",
            0, 0,
            games,
            bufferSize * count
            );

        if (!returnValue)
        {
            Memory_Free(games);
        }

    } while ((returnValue == ((bufferSize * count) - 1))
        || (returnValue == ((bufferSize * count) - 2)));

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
