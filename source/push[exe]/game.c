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


BOOLEAN SearchGame( WCHAR* ExecutablePath, WCHAR* GameId )
{
    // Try searching for names that match. If a match is found, compare the executable's checksum. We do this to find
    // games that might have changed paths. As Push uses a direct file path hueristic to determine game settings to
    // use, users might run into problems trying to figure out why their games settings don't work while not noticing
    // that their game executable has changed paths. This function updates the path and gets everything back to
    // running nicely again.

    DWORD headerSum;
    DWORD checkSum;
    GAME_LIST gameList = Game_GetGames();
    wchar_t *executable = String_FindLastChar(ExecutablePath, '\\');

    executable++;

    while (gameList != NULL)
    {
        if (String_Compare(gameList->Game->ExecutableName, executable) == 0)
        {
            MapFileAndCheckSumW(ExecutablePath, &headerSum, &checkSum);
            Log(L"MapFileAndCheckSumW(%s, 0x%X)", ExecutablePath, checkSum);
            Log(L"gameList->Game->CheckSum: 0x%X", gameList->Game->CheckSum);

            if (gameList->Game->CheckSum == checkSum)
            {
                Log(L"Found game executable at another path!");
                
                if (GameId)
                {
                    String_Copy(GameId, gameList->Game->Id);
                }

                // Update path. 
                SlIniWriteString(L"Games", gameList->Game->ExecutablePath, NULL);
                SlIniWriteString(L"Games", ExecutablePath, gameList->Game->Id);

                return TRUE;
            }
        }

        gameList = gameList->NextEntry;
    }

    return FALSE;
}


BOOLEAN Game_IsGame( WCHAR* ExecutablePath )
{
    wchar_t buffer[260];
    DWORD result;

    Log(L"IsGame(%s)", ExecutablePath);

    result = Ini_GetString(L"Games", ExecutablePath, 0, buffer, 260);

    if (result)
    {
        //is game
        return TRUE;
    }
    else
    {
        return SearchGame(ExecutablePath, NULL);
    }

    return FALSE;
}


VOID Game_Initialize( WCHAR* Win32Name, PUSH_GAME* Game )
{
    WCHAR buffer[260];
    WCHAR installPath[260];
    WCHAR *lastSlash;
    DWORD start;
    DWORD end;
    DWORD len;

    if (!Win32Name)
        return;

    Log(L"Game_Initialize(%s)", Win32Name);
    
    Game->ExecutablePath = HeapedString(Win32Name);
    lastSlash = String_FindLastChar(Game->ExecutablePath, '\\');
    Game->ExecutableName = lastSlash + 1;

    Ini_GetString(L"Games", Game->ExecutablePath, NULL, Game->Id, sizeof(Game->Id) / sizeof(WCHAR));
    Log(L"gameId: %s", Game->Id);

    if (String_Compare(Game->Id, L"") == 0)
    {
        Log(L"Bad game id! game must have changed paths!");
        SearchGame(Win32Name, Game->Id);
        Log(L"Found game at new gameId: %s", Game->Id);
    }

    Ini_ReadSubKey(
        L"Game Settings", 
        Game->Id,
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
        Game->Id,
        GAME_INSTALL_PATH, 
        installPath, 
        buffer, 
        260
        );
    
    Game->InstallPath = HeapedString(buffer);

    // Get checksum
    Ini_ReadSubKey(L"Game Settings", Game->Id, L"CheckSum", NULL, buffer, 260);
    
    if (buffer)
    {
        Game->CheckSum = wcstol(buffer, NULL, 16);
    }
    
    if (Game->CheckSum == 0)
    {
        //update checksum
        DWORD headerSum, checkSum;

        Log(L"Updating checksum for %s", Game->ExecutablePath);
        MapFileAndCheckSumW(Game->ExecutablePath, &headerSum, &checkSum);
        Game_SetCheckSum(Game, checkSum);
    }

    // Game Settings.

    if (Ini_ReadSubKeyBoolean(L"Game Settings", Game->Id, L"DisableOverlay", FALSE))
    {
        Game->Settings.DisableOverlay = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", Game->Id, L"UseRamDisk", FALSE))
    {
        Game->Settings.UseRamDisk = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", Game->Id, L"DisableRepeatKeys", FALSE))
    {
        Game->Settings.DisableRepeatKeys = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", Game->Id, L"SwapWASD", FALSE))
    {
        Game->Settings.SwapWASD = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", Game->Id, L"ForceMaxClocks", FALSE))
    {
        Game->Settings.ForceMaxClocks = TRUE;
    }

    if (Ini_ReadSubKeyBoolean(L"Game Settings", Game->Id, L"Patch", FALSE))
    {
        Game->Settings.PatchMemory = TRUE;
    }

    Ini_ReadSubKey(L"Game Settings", Game->Id, L"ForceVsync", NULL, buffer, 260);

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
            return NULL;
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
