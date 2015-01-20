#ifndef GAMES_H
#define GAMES_H


//Game Flags
#define GAME_RAMDISK        0x00000001
#define GAME_VSYNC          0x00000002
#define GAME_REPEAT_KEYS    0x00000004
#define GAME_WASD           0x00000008


class PushGame{
    WCHAR* GameWin32Name;
public:
    PushGame( WCHAR* Game );
    WCHAR* GetName();
    VOID SetName( WCHAR* Name );
    WCHAR* GetInstallPath();
    VOID SetInstallPath( WCHAR* Path );
    DWORD GetFlags();
    VOID SetFlags( DWORD Flags );
};

#endif //GAMES_H