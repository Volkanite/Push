#ifndef RAMDISK_H
#define RAMDISK_H

//
// Callback command types
//
typedef enum {
    PROGRESS,
    DONEWITHSTRUCTURE,
    UNKNOWN2,
    UNKNOWN3,
    UNKNOWN4,
    UNKNOWN5,
    INSUFFICIENTRIGHTS,
    UNKNOWN7,
    UNKNOWN8,
    UNKNOWN9,
    UNKNOWNA,
    DONE,
    UNKNOWNC,
    UNKNOWND,
    OUTPUT,
    STRUCTUREPROGRESS
} CALLBACKCOMMAND;

//
// FMIFS callback definition
//
typedef BOOLEAN (__stdcall *PFMIFSCALLBACK)( CALLBACKCOMMAND Command,
                                                   DWORD SubAction,
                                                   void *ActionInfo );


//
// Format command in FMIFS
//

// media flags
#define FMIFS_HARDDISK 0xC
#define FMIFS_FLOPPY   0x8

typedef void (__stdcall *PFORMATEX)( WCHAR* DriveRoot,
                          DWORD     MediaFlag,
                          WCHAR* Format,
                          WCHAR* Label,
                          SDWORD    QuickFormat,
                          DWORD     ClusterSize,
                          PFMIFSCALLBACK Callback );
#ifdef __cplusplus
extern "C" {
#endif

void
FormatRamDisk();

void
RemoveRamDisk();

void
CreateRamDisk(
    UINT32  iBytes,
    char    cMountPoint
    );

wchar_t
__stdcall
FindFreeDriveLetter();

#ifdef __cplusplus
}
#endif

#endif //RAMDISK_H





