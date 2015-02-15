#ifndef GAMES_H
#define GAMES_H


//Game Flags
#define GAME_RAMDISK        0x00000001
#define GAME_VSYNC          0x00000002
#define GAME_REPEAT_KEYS    0x00000004
#define GAME_WASD           0x00000008


typedef struct _PUSH_GAME_SETTINGS
{
    BOOLEAN                     UseRamDisk;
    BOOLEAN                     DisableRepeatKeys;
    BOOLEAN                     SwapWASD;
    PUSH_VSYNC_OVERRIDE_MODE    VsyncOverrideMode;

} PUSH_GAME_SETTINGS;

typedef struct _PUSH_GAME
{
    WCHAR*              Win32Name;
    WCHAR*              Name;
    WCHAR*              InstallPath;
    PUSH_GAME_SETTINGS  GameSettings;

}PUSH_GAME;


#ifdef __cplusplus
extern "C" {
#endif

VOID Game_Initialize( 
    WCHAR* Win32Name, 
    PUSH_GAME* Game 
    );

VOID Game_SetName( 
    PUSH_GAME* Game, 
    WCHAR* Name 
    );

VOID Game_SetInstallPath( 
    PUSH_GAME *Game, 
    WCHAR* Path 
    );

VOID Game_SetFlags( 
    PUSH_GAME *Game, 
    DWORD Flags 
    );

#ifdef __cplusplus
}
#endif


#endif //GAMES_H