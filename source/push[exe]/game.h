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
    BOOLEAN                     ForceMaxClocks;
    PUSH_VSYNC_OVERRIDE_MODE    VsyncOverrideMode;

} PUSH_GAME_SETTINGS;

typedef struct _PUSH_GAME
{
    WCHAR* ExecutableName;
    WCHAR* ExecutablePath;
    WCHAR* Name;
    WCHAR* InstallPath;
    WCHAR Id[2];
    DWORD CheckSum;
    PUSH_GAME_SETTINGS Settings;

}PUSH_GAME;

typedef struct _GAME_LIST_ENTRY GAME_LIST_ENTRY;
typedef struct _GAME_LIST_ENTRY
{
    PUSH_GAME* Game;
    GAME_LIST_ENTRY* NextEntry;

} GAME_LIST_ENTRY, *GAME_LIST;


class Game{

public:
	static VOID Initialize(WCHAR* Win32Name, PUSH_GAME* Game);
	static VOID SetCheckSum(PUSH_GAME* Game, DWORD CheckSum);
	static VOID SetFlags(PUSH_GAME *Game, DWORD Flags);
	static VOID SetInstallPath(PUSH_GAME *Game, WCHAR* Path);
	static VOID SetName(PUSH_GAME* Game, WCHAR* Name);
	static GAME_LIST GetGames();
};


#endif //GAMES_H